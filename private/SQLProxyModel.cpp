#include "IzSQLUtilities/SQLProxyModel.h"

#include <QtConcurrent>

#include "IzSQLUtilities/SQLDataContainer.h"
#include "IzSQLUtilities/SQLModel.h"
#include "IzSQLUtilities/SQLQueryBuilder.h"

IzSQLUtilities::SQLProxyModel::SQLProxyModel(QObject* parent)
	: QSortFilterProxyModel(parent)
{
	// watchers setup
	m_filterWatcher = new QFutureWatcher<QSet<int>>(this);
	connect(m_filterWatcher, &QFutureWatcher<QSet<int>>::finished, this, &IzSQLUtilities::SQLProxyModel::onDataFiltered);

	// additional connects for concurrent operations
	connect(this, &IzSQLUtilities::SQLProxyModel::filterChanged, this, &IzSQLUtilities::SQLProxyModel::onFilterChangeRequested);

	qRegisterMetaType<IzSQLUtilities::SQLProxyModel*>();
}

IzSQLUtilities::SQLModel* IzSQLUtilities::SQLProxyModel::sourceModel(bool isInitializing) const
{
	if (!isInitializing) {
		checkSource();
	}
	return m_sourceModel;
}

void IzSQLUtilities::SQLProxyModel::setupSource(SQLModel* source)
{
	if (source != nullptr) {
		m_sourceModel = source;
		qInfo() << "Connecting to existing source model.";
	} else {
		m_sourceModel = new SQLModel(this);
		qInfo() << "Initializing new source model.";
	}
	setSourceModel(sourceModel(true));
	setupConnects();
}

bool IzSQLUtilities::SQLProxyModel::checkSource(bool initialization) const
{
	if (m_sourceModel != nullptr) {
		if (!m_connectedToSource) {
			qCritical() << "Proxy model is not correctly connected to source.";
			return false;
		}
		return true;
	}
	if (!initialization) {
		qCritical() << "Source model is not initialized.";
	}
	return false;
}

void IzSQLUtilities::SQLProxyModel::onSourceModelDataAboutToBeChanged(int row, const QString& columnName, const QVariant& newFieldValue)
{
	if (m_filters.empty() || !m_filters.contains(columnName)) {
		return;
	}
	if (m_filters.value(columnName).second) {
		if (newFieldValue != m_filters.value(columnName).first) {
			m_filteredIndexes.remove(row);
			return;
		}
	} else {
		if (!newFieldValue.toString().contains(m_filters.value(columnName).first.toString(), Qt::CaseInsensitive)) {
			m_filteredIndexes.remove(row);
			return;
		}
	}
	m_filteredIndexes.insert(row);
}

bool IzSQLUtilities::SQLProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
	Q_UNUSED(sourceParent)
	if (m_filters.empty()) {
		return true;
	}
	return m_filteredIndexes.contains(sourceRow);
}

void IzSQLUtilities::SQLProxyModel::onSourceModelRowAboutToBeAdded(int row, const std::vector<QVariant>& fieldValues)
{
	if (m_filters.empty()) {
		return;
	}
	QHashIterator<QString, QPair<QVariant, bool>> i(m_filters);
	auto hits = 0;
	while (i.hasNext()) {
		i.next();
		auto cIdx = sourceModel()->getColumnIndex(i.key());
		if (cIdx == -1) {
			return;
		}
		if (i.value().second) {
			if (fieldValues.at(cIdx) == i.value().first) {
				hits++;
			}
		} else {
			if (fieldValues.at(cIdx).toString().contains(i.value().first.toString(), Qt::CaseInsensitive)) {
				hits++;
			}
		}
	}
	if (hits == m_filters.size()) {
		m_filteredIndexes.insert(row);
	}
}

void IzSQLUtilities::SQLProxyModel::onDataFiltered()
{
	if (!m_filterWatcher->isCanceled()) {
		m_filteredIndexes = m_filterWatcher->result();
		clearCachedSourceData();
		beginResetModel();
		endResetModel();
		emit filteringEnded();
	}
}

void IzSQLUtilities::SQLProxyModel::onFilterChangeRequested()
{
	if (sourceModel()->isInLoadProcess()) {
		return;
	}
	if (checkSource() && m_filterEnabled) {
		if (m_filterWatcher->isRunning()) {
			m_filterWatcher->cancel();
			m_filterWatcher->waitForFinished();
		} else {
			emit filteringStarted();
		}
		cacheSourceData();

		QSet<int> sequence;
		sequence.reserve(m_clonedData.size());
		for (auto i{ 0 }; i < m_clonedData.size(); ++i) {
			sequence.insert(i);
		}

		// TODO: sprawdzić warunki na data race
		QFuture<QSet<int>> filterData = QtConcurrent::run([this]() -> QSet<int> {
			QSet<int> sequence;
			for (int row = 0; row < m_clonedData.size(); ++row) {
				if (m_filterWatcher->isCanceled()) {
					return {};
				}
				QHashIterator<QString, QPair<QVariant, bool>> i(m_filters);
				int hits{ 0 };
				while (i.hasNext()) {
					i.next();
					if (i.value().second) {
						if (m_clonedData[row]->fieldValue(m_cachedColumnIndexMap.value(i.key(), -1)) == i.value().first) {
							sequence.insert(row);
							hits++;
						}
					} else {
						if (m_clonedData[row]->fieldValue(m_cachedColumnIndexMap.value(i.key(), -1)).toString().contains(i.value().first.toString(), Qt::CaseInsensitive)) {
							hits++;
						}
					}
				}
				if (hits == m_filters.size()) {
					sequence.insert(row);
				}
			};
			return sequence;
		});
		m_filterWatcher->setFuture(filterData);
	}
}

void IzSQLUtilities::SQLProxyModel::onSourceRefreshCompleted()
{
	if (!m_filters.empty()) {
		onFilterChangeRequested();
	}
}

void IzSQLUtilities::SQLProxyModel::onSourceRefreshStarted()
{
	if (m_filterWatcher->isRunning()) {
		m_filterWatcher->cancel();
	}
	m_filteredIndexes.clear();
}

void IzSQLUtilities::SQLProxyModel::cacheSourceData()
{
	clearCachedSourceData();
	m_clonedData = m_sourceModel->cloneData();
	m_clonedData.detach();
	m_cachedColumnIndexMap = m_sourceModel->getColumnIndexMap();
	m_cachedColumnIndexMap.detach();
}

void IzSQLUtilities::SQLProxyModel::clearCachedSourceData()
{
	m_clonedData.clear();
	m_cachedColumnIndexMap.clear();
}

void IzSQLUtilities::SQLProxyModel::addFilter(const QString& columnName, const QVariant& filterValue, bool exactValue)
{
	m_filters.insert(columnName, QPair<QVariant, bool>{ filterValue, exactValue });
	emit filterChanged(filterValue);
}

void IzSQLUtilities::SQLProxyModel::removeFilter(const QString& columnName)
{
	m_filters.remove(columnName);
	emit filterChanged({});
}

void IzSQLUtilities::SQLProxyModel::clearFilters()
{
	m_filters.clear();
	emit filterChanged({});
}

bool IzSQLUtilities::SQLProxyModel::isEmpty() const
{
	return (!index(0, 0).isValid());
}

const QVariant IzSQLUtilities::SQLProxyModel::getFilterValue(const QString& columnName) const
{
	return m_filters.value(columnName).first;
}

bool IzSQLUtilities::SQLProxyModel::getFilterEnabled() const
{
	return m_filterEnabled;
}

void IzSQLUtilities::SQLProxyModel::setFilterEnabled(bool filterEnabled)
{
	m_filterEnabled = filterEnabled;
}

void IzSQLUtilities::SQLProxyModel::setSource(SQLModel* sourceModel)
{
	if (sourceModel == nullptr) {
		qCritical() << "Got invalid source model.";
		return;
	}
	m_sourceModel = sourceModel;
}

void IzSQLUtilities::SQLProxyModel::setupConnects()
{
	if (m_sourceModel == nullptr) {
		qCritical() << "Source model is not defined.";
		return;
	}
	connect(m_sourceModel, &SQLModel::refreshStarted, this, &IzSQLUtilities::SQLProxyModel::onSourceRefreshStarted, Qt::DirectConnection);
	connect(m_sourceModel, &SQLModel::refreshCompleted, this, &IzSQLUtilities::SQLProxyModel::onSourceRefreshCompleted);
	connect(m_sourceModel, &SQLModel::dataAboutToBeChanged, this, &IzSQLUtilities::SQLProxyModel::onSourceModelDataAboutToBeChanged);
	connect(m_sourceModel, &SQLModel::rowAboutToBeAdded, this, &IzSQLUtilities::SQLProxyModel::onSourceModelRowAboutToBeAdded);
	m_connectedToSource = true;
}

int IzSQLUtilities::SQLProxyModel::sourceRow(int proxyRow) const
{
	QModelIndex tmpIndex = index(proxyRow, 0);
	if (tmpIndex.isValid()) {
		return mapToSource(tmpIndex).row();
	}
	qCritical() << "Invalid proxy model row:" << proxyRow << "requested.";
	return -1;
}

int IzSQLUtilities::SQLProxyModel::proxyRow(int sourceRow) const
{
	QModelIndex tmpIndex = m_sourceModel->index(sourceRow, 0, QModelIndex());
	if (tmpIndex.isValid()) {
		return mapFromSource(tmpIndex).row();
	}
	qCritical() << "Invalid source model row:" << sourceRow << "requested.";
	return -1;
}

void IzSQLUtilities::SQLProxyModel::invertState(bool isAdded)
{
	for (auto i = 0; i < rowCount(); ++i) {
		if (isAdded) {
			sourceModel()->flipIsAdded(sourceRow(i));
		} else {
			sourceModel()->flipToBeRemoved(sourceRow(i));
		}
	}
}

void IzSQLUtilities::SQLProxyModel::setState(bool isAdded)
{
	for (auto i = 0; i < rowCount(); ++i) {
		if (isAdded) {
			if (!sourceModel()->isAdded(sourceRow(i))) {
				sourceModel()->flipIsAdded(sourceRow(i));
			}
		} else {
			if (!sourceModel()->toBeRemoved(sourceRow(i))) {
				sourceModel()->flipToBeRemoved(sourceRow(i));
			}
		}
	}
}

void IzSQLUtilities::SQLProxyModel::removeState(bool isAdded)
{
	for (auto i = 0; i < rowCount(); ++i) {
		if (isAdded) {
			if (sourceModel()->isAdded(sourceRow(i))) {
				sourceModel()->flipIsAdded(sourceRow(i));
			}
		} else {
			if (sourceModel()->toBeRemoved(sourceRow(i))) {
				sourceModel()->flipToBeRemoved(sourceRow(i));
			}
		}
	}
}

void IzSQLUtilities::SQLProxyModel::normalizeState(bool isAdded, bool state)
{
	for (auto i = 0; i < rowCount(); ++i) {
		if (isAdded) {
			if (sourceModel()->isAdded(sourceRow(i)) != state) {
				sourceModel()->flipIsAdded(sourceRow(i));
			}
		} else {
			if (sourceModel()->toBeRemoved(sourceRow(i)) != state) {
				sourceModel()->flipToBeRemoved(sourceRow(i));
			}
		}
	}
}

void IzSQLUtilities::SQLProxyModel::sortColumn(const QString& columnName, bool descending)
{
	if (!checkSource()) {
		qCritical() << "Source model is not defined.";
		return;
	}
	emit aboutToSort(columnName, descending);
	setSortRole(m_sourceModel->roleNames().key(columnName.toLocal8Bit()));
	sort(0, descending ? Qt::DescendingOrder : Qt::AscendingOrder);
	emit dataSorted(columnName, descending);
}
