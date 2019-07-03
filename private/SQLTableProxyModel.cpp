#include "IzSQLUtilities/SQLTableProxyModel.h"

#include <QDebug>
#include <QItemSelectionModel>
#include <QThread>
#include <QtConcurrent>

#include "IzSQLUtilities/SQLDataContainer.h"
#include "IzSQLUtilities/SQLTableModel.h"

IzSQLUtilities::SQLTableProxyModel::SQLTableProxyModel(QObject* parent)
	: QSortFilterProxyModel(parent)
	, m_sourceModel(new SQLTableModel(this))
{
	// default sort role
	setSortRole(static_cast<int>(SQLTableModel::SQLTableModelRoles::DisplayData));

	// watchers setup
	m_filterFutureWatcher = new QFutureWatcher<QSet<int>>(this);
	connect(m_filterFutureWatcher, &QFutureWatcher<QSet<int>>::finished, this, &SQLTableProxyModel::onDataFiltered);

	// we don't really use sourceModel parameter in this function
	setSourceModel(nullptr);

	// source model connects
	connect(m_sourceModel, &SQLTableModel::dataRefreshEnded, this, [this]() {
		// ony reset filtering if model executed new query
		if (m_sourceModel->executedNewQuery()) {
			m_filtersApplied = false;
			m_filteredIndexes.clear();
			m_filters.clear();
		} else {
			filterData();
		}
	});

	connect(this, &SQLTableProxyModel::isFilteringChanged, this, [this]() {
		if (isFiltering()) {
			m_sourceModel->layoutAboutToBeChanged();
		} else {
			m_sourceModel->layoutChanged();
		}
	});
}

void IzSQLUtilities::SQLTableProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
	Q_UNUSED(sourceModel)
	if (this->sourceModel() != nullptr) {
		qCritical() << "SQLTableProxyModel automatically sets source model to the internal instance of SQLTableModel. Overriding this functionality is not supported.";
	} else {
		QSortFilterProxyModel::setSourceModel(m_sourceModel);
	}
}

bool IzSQLUtilities::SQLTableProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
	Q_UNUSED(source_parent)
	return m_filtersApplied ? m_filteredIndexes.contains(source_row) : true;
}

bool IzSQLUtilities::SQLTableProxyModel::filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const
{
	Q_UNUSED(source_parent)
	return !m_hiddenColumns.contains(source_column) && !m_excludedColumns.contains(source_column);
}

QSet<int> IzSQLUtilities::SQLTableProxyModel::hiddenColumns() const
{
	return m_hiddenColumns;
}

void IzSQLUtilities::SQLTableProxyModel::setHiddenColumns(const QSet<int> &hiddenColumns)
{
	m_hiddenColumns = hiddenColumns;
	invalidate();
}

QSet<int> IzSQLUtilities::SQLTableProxyModel::excludedColumns() const
{
	return m_excludedColumns;
}

void IzSQLUtilities::SQLTableProxyModel::setExcludedColumns(const QSet<int>& excludedColumns)
{
	if (m_excludedColumns != excludedColumns) {
		m_excludedColumns = excludedColumns;
		invalidate();
	}
}

int IzSQLUtilities::SQLTableProxyModel::sourceRow(int proxyRow) const
{
	return mapToSource(index(proxyRow, 0)).row();
}

int IzSQLUtilities::SQLTableProxyModel::sourceColumn(int proxyColumn) const
{
	return mapToSource(index(0, proxyColumn)).column();
}

int IzSQLUtilities::SQLTableProxyModel::proxyRow(int sourceRow) const
{
	return mapFromSource(m_sourceModel->index(sourceRow, 0)).row();
}

int IzSQLUtilities::SQLTableProxyModel::proxyColumn(int sourceColumn) const
{
	return mapFromSource(m_sourceModel->index(0, sourceColumn)).column();
}

QModelIndex IzSQLUtilities::SQLTableProxyModel::sourceIndex(int proxyRow, int proxyColumn) const
{
	return mapToSource(index(proxyRow, proxyColumn));
}

QModelIndex IzSQLUtilities::SQLTableProxyModel::proxyIndex(int sourceRow, int sourceColumn) const
{
	return mapFromSource(m_sourceModel->index(sourceRow, sourceColumn));
}

void IzSQLUtilities::SQLTableProxyModel::onDataFiltered()
{
	if (!m_filterFutureWatcher->isCanceled()) {
		m_isFiltering     = false;
		m_filteredIndexes = m_filterFutureWatcher->result();
		invalidateFilter();
		emit isFilteringChanged();
	}
}

bool IzSQLUtilities::SQLTableProxyModel::isFiltering() const
{
	return m_isFiltering;
}

void IzSQLUtilities::SQLTableProxyModel::addColumnFilter(int column, const QRegularExpression& filter)
{
	m_filters.insert(column, filter);
	m_filtersApplied = true;
	filterData();
}

void IzSQLUtilities::SQLTableProxyModel::removeColumnFilter(int column)
{
	m_filters.remove(column);
	m_filtersApplied = !m_filters.empty();
	filterData();
}

void IzSQLUtilities::SQLTableProxyModel::clearColumnFilters()
{
	m_filters.clear();
	m_filtersApplied = false;
	filterData();
}

QItemSelectionModel* IzSQLUtilities::SQLTableProxyModel::selectionModel() const
{
	return m_selectionModel;
}

void IzSQLUtilities::SQLTableProxyModel::setSelectionModel(QItemSelectionModel* selectionModel)
{
	m_selectionModel = selectionModel;
}

void IzSQLUtilities::SQLTableProxyModel::changeColumnVisibilitiy(int column, bool visibility)
{
	if (visibility) {
		m_hiddenColumns.remove(column);
	} else {
		m_hiddenColumns.insert(column);
	}
	invalidateFilter();
}

void IzSQLUtilities::SQLTableProxyModel::filterData()
{
	// check state of source model
	if (m_sourceModel->isRefreshingData()) {
		qWarning() << "filterData() called during model refreshing.";
		return;
	}

	// if watcher is running, cancel it
	if (m_filterFutureWatcher->isRunning()) {
		m_filterFutureWatcher->cancel();
		m_filterFutureWatcher->waitForFinished();
	}

	// set filtering state
	m_isFiltering = true;
	emit isFilteringChanged();

	// if filters are empty reset filtring
	if (m_filters.isEmpty()) {
		m_filteredIndexes.clear();
		m_isFiltering = false;
		emit isFilteringChanged();
		invalidateFilter();
		return;
	}

	// cache filters and generate index set
	m_cachedFilters = m_filters;
	QSet<int> indexes;
	for (int i{ 0 }; i < m_sourceModel->rowCount(); ++i) {
		indexes.insert(i);
	}

	// launch concurrent filtering
	QFuture<QSet<int>> filteredData = QtConcurrent::filteredReduced<QSet<int>>(indexes,
																			   [this](int row) {
																				   int hits{ 0 };
																				   QHashIterator<int, QRegularExpression> it(m_cachedFilters);
																				   while (it.hasNext()) {
																					   it.next();
																					   if (m_sourceModel->at(row).columnValue(it.key()).toString().contains(it.value())) {
																						   hits++;
																					   }
																				   }
																				   return hits == m_cachedFilters.size();
																			   },
																			   [](QSet<int>& set, int row) {
																				   set.insert(row);
																			   });
	m_filterFutureWatcher->setFuture(filteredData);
}

IzSQLUtilities::SQLTableModel* IzSQLUtilities::SQLTableProxyModel::source() const
{
	return m_sourceModel;
}

QVariant IzSQLUtilities::SQLTableProxyModel::data(const QModelIndex& index, int role) const
{
	if (m_selectionModel != nullptr && role == static_cast<int>(SQLTableModel::SQLTableModelRoles::IsSelected)) {
		return m_selectionModel->isSelected(index);
	}
	return QSortFilterProxyModel::data(index, role);
}

QVariant IzSQLUtilities::SQLTableProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return m_sourceModel->headerData(sourceColumn(section), orientation, role);
}
