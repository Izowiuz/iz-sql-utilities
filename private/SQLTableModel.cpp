#include "IzSQLUtilities/SQLTableModel.h"

#include <QDebug>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QtConcurrent>

#include "IzSQLUtilities/SQLdbc.h"

#include "LoadedSQLData.h"

IzSQLUtilities::SQLTableModel::SQLTableModel(QObject* parent)
	: QAbstractItemModel(parent)
{
	// watchers setup
	m_refreshFutureWatcher = new QFutureWatcher<LoadedData>(this);
	connect(m_refreshFutureWatcher, &QFutureWatcher<LoadedData>::finished, this, &SQLTableModel::parseSQLData);
}

QVariant IzSQLUtilities::SQLTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(role)
	if (orientation == Qt::Horizontal) {
		return m_columnNameColumnAliasMap.contains(columnNameFromIndex(section)) ? m_columnNameColumnAliasMap[columnNameFromIndex(section)] : m_indexColumnMap[section];
	}
	return section;
}

bool IzSQLUtilities::SQLTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
	if (orientation == Qt::Horizontal) {
		if (section < columnCount() && value != headerData(section, orientation, role)) {
			m_columnNameColumnAliasMap.insert(columnNameFromIndex(section), value.toString());
			emit headerDataChanged(orientation, section, section);
			return true;
		}
	}
	qWarning() << "Currently setting vertical header data is not supported";
	return false;
}

QModelIndex IzSQLUtilities::SQLTableModel::index(int row, int column, const QModelIndex& parent) const
{
	if (hasIndex(row, column, parent)) {
		return createIndex(row, column);
	}
	return {};
}

QModelIndex IzSQLUtilities::SQLTableModel::parent(const QModelIndex& index) const
{
	Q_UNUSED(index)
	return {};
}

int IzSQLUtilities::SQLTableModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_data.size();
}

int IzSQLUtilities::SQLTableModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_indexColumnMap.size();
}

bool IzSQLUtilities::SQLTableModel::hasChildren(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return {};
}

bool IzSQLUtilities::SQLTableModel::canFetchMore(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return false;
}

void IzSQLUtilities::SQLTableModel::fetchMore(const QModelIndex& parent){
	Q_UNUSED(parent)
}

QVariant IzSQLUtilities::SQLTableModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) {
		return {};
	}
	switch (static_cast<SQLTableModel::SQLTableModelRoles>(role)) {
	case SQLTableModel::SQLTableModelRoles::DisplayData:
		return m_data[index.row()]->fieldValue(index.column());
	case SQLTableModel::SQLTableModelRoles::IsAdded:
		return m_data[index.row()]->isAdded();
	case SQLTableModel::SQLTableModelRoles::ToBeRemoved:
		return m_data[index.row()]->toBeRemoved();
	case SQLTableModel::SQLTableModelRoles::IsDirty:
		return m_data[index.row()]->isDirty();
	case SQLTableModel::SQLTableModelRoles::IsValid:
		return m_data[index.row()]->isValid();
	default:
		return {};
	}
}

bool IzSQLUtilities::SQLTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (data(index, role) != value) {
		// FIXME: Implement me!
		emit dataChanged(index, index, QVector<int>() << role);
		return true;
	}
	return false;
}

Qt::ItemFlags IzSQLUtilities::SQLTableModel::flags(const QModelIndex& index) const
{
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}
	return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool IzSQLUtilities::SQLTableModel::insertRows(int row, int count, const QModelIndex& parent)
{
	beginInsertRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endInsertRows();
	return {};
}

bool IzSQLUtilities::SQLTableModel::insertColumns(int column, int count, const QModelIndex& parent)
{
	beginInsertColumns(parent, column, column + count - 1);
	// FIXME: Implement me!
	endInsertColumns();
	return {};
}

bool IzSQLUtilities::SQLTableModel::removeRows(int row, int count, const QModelIndex& parent)
{
	beginRemoveRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endRemoveRows();
	return {};
}

bool IzSQLUtilities::SQLTableModel::removeColumns(int column, int count, const QModelIndex& parent)
{
	beginRemoveColumns(parent, column, column + count - 1);
	// FIXME: Implement me!
	endRemoveColumns();
	return {};
}

QHash<int, QByteArray> IzSQLUtilities::SQLTableModel::roleNames() const
{
	m_cachedRoleNames.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::DisplayData), QByteArrayLiteral("displayData"));
	m_cachedRoleNames.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::IsAdded), QByteArrayLiteral("isAdded"));
	m_cachedRoleNames.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::ToBeRemoved), QByteArrayLiteral("toBeRemoved"));
	m_cachedRoleNames.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::IsDirty), QByteArrayLiteral("isDirty"));
	m_cachedRoleNames.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::IsValid), QByteArrayLiteral("isValid"));
	m_cachedRoleNames.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::IsSelected), QByteArrayLiteral("isSelected"));
	return m_cachedRoleNames;
}

QString IzSQLUtilities::SQLTableModel::columnNameFromIndex(int index) const
{
	return m_indexColumnMap.value(index);
}

int IzSQLUtilities::SQLTableModel::indexFromColumnName(const QString& column) const
{
	return m_columnIndexMap.value(column, -1);
}

bool IzSQLUtilities::SQLTableModel::validateSqlQuery(const QString& sqlQuery, const QVariantMap& sqlParameters) const
{
	const int queryStringSize = sqlQuery.size();
	int delimiterStart{ -1 };
	int delimiterEnd{ -1 };
	int parametersFound{ 0 };
	for (auto i = 0; i < queryStringSize; ++i) {
		if (sqlQuery[i] == QStringLiteral("'") && i < queryStringSize - 1 && sqlQuery[i + 1] == QStringLiteral(":")) {
			delimiterStart = i;
		} else if (delimiterStart != -1 && sqlQuery[i] == QStringLiteral("'")) {
			delimiterEnd = i;
		}
		if (delimiterStart != -1 && delimiterEnd != -1) {
			auto str = sqlQuery.mid(delimiterStart + 1, delimiterEnd - delimiterStart - 1);
			if (!sqlParameters.contains(str)) {
				qCritical() << "Parameter" << str << "not found in passed parameters.";
				return false;
			}
			parametersFound++;
			delimiterStart = -1;
			delimiterEnd   = -1;
		}
	}
	if (delimiterStart != -1 && delimiterEnd == -1) {
		qCritical() << "Unterminated delimiter. Delimiter start position:" << delimiterStart;
		return false;
	}
	if (parametersFound != sqlParameters.size()) {
		qCritical() << "Number of parameters in query are not equal to passed parameters.";
		return false;
	}
	return true;
}

QString IzSQLUtilities::SQLTableModel::normalizeSqlQuery(const QString& sqlQuery, const QVariantMap& sqlParameters) const
{
	QString tmp = sqlQuery;
	QMapIterator<QString, QVariant> it(sqlParameters);
	while (it.hasNext()) {
		it.next();
		tmp.replace(QStringLiteral("'") + it.key() + QStringLiteral("'"), it.key());
	}
	return tmp;
}

void IzSQLUtilities::SQLTableModel::setIsRefreshing(bool isRefreshing)
{
	if (m_isRefreshing != isRefreshing) {
		m_isRefreshing = isRefreshing;
		emit isRefreshingChanged();
	}
}

bool IzSQLUtilities::SQLTableModel::queryIsValid() const
{
	return m_queryIsValid;
}

QVariantMap IzSQLUtilities::SQLTableModel::columnNameColumnAliasMap() const
{
	return m_columnNameColumnAliasMap;
}

void IzSQLUtilities::SQLTableModel::setColumnNameColumnAliasMap(const QVariantMap& columnNameColumnAliasMap)
{
	m_columnNameColumnAliasMap = columnNameColumnAliasMap;
}

void IzSQLUtilities::SQLTableModel::setQueryIsValid(bool queryIsValid)
{
	if (m_queryIsValid != queryIsValid) {
		m_queryIsValid = queryIsValid;
		emit queryIsValidChanged();
	}
}

void IzSQLUtilities::SQLTableModel::parseSQLData()
{
	if (std::get<0>(m_refreshFutureWatcher->result()) == SQLTableModel::DataRefreshResult::Refreshed) {
		beginResetModel();
		m_columnIndexMap = std::get<2>(m_refreshFutureWatcher->result())->columnIndexMap();
		m_indexColumnMap = std::get<2>(m_refreshFutureWatcher->result())->indexColumnMap();
		m_data.swap(std::get<2>(m_refreshFutureWatcher->result())->sqlData());
		endResetModel();
		setIsRefreshing(false);
		emit refreshEnded(true);
	} else {
		beginResetModel();
		endResetModel();
		setIsRefreshing(false);
		emit refreshEnded(false);
	}
}

IzSQLUtilities::SQLTableModel::LoadedData IzSQLUtilities::SQLTableModel::fullDataRefresh(const QString& sqlQuery, const QVariantMap& sqlParameters)
{
	// database connect
	SQLdbc db(QStringLiteral("model-refresh"));
	if (!db.initializeConnection()) {
		return { SQLTableModel::DataRefreshResult::DatabaseError, SQLTableModel::DataRefreshType::Full, std::shared_ptr<LoadedSQLData>() };
	}

	// qsql query setup
	QSqlQuery query(db.getConnection());
	query.setForwardOnly(true);
	query.prepare(sqlQuery);

	QMapIterator<QString, QVariant> it(sqlParameters);
	while (it.hasNext()) {
		it.next();
		query.bindValue(it.key(), it.value());
	}

	// query exec
	if (!query.exec()) {
		return { SQLTableModel::DataRefreshResult::QueryError, SQLTableModel::DataRefreshType::Full, std::shared_ptr<LoadedSQLData>() };
	}

	// additional data
	int rowCount     = 0;
	int columnsCount = query.record().count();

	QHash<QString, int> columnIndexMap;
	columnIndexMap.reserve(columnsCount);

	QHash<int, QString> indexColumnMap;
	indexColumnMap.reserve(columnsCount);

	for (int i = 0; i < columnsCount; i++) {
		columnIndexMap.insert(query.record().fieldName(i), i);
		indexColumnMap.insert(i, query.record().fieldName(i));
	}

	auto sqlData = std::make_shared<LoadedSQLData>();
	sqlData->setColumnIndexMap(columnIndexMap);
	sqlData->setIndexColumnMap(indexColumnMap);

	emit rowsLoaded(0);

	// query data
	while (query.next()) {
		if (rowCount % 100 == 0) {
			emit rowsLoaded(rowCount);
		}

		auto row = std::make_unique<SQLDataContainer>(static_cast<unsigned int>(columnsCount));
		row->setIsInitializing(true);
		for (int i = 0; i < columnsCount; ++i) {
			row->addField(query.value(i));
		}
		row->setIsInitializing(false);

		sqlData->addRow(std::move(row));
		rowCount++;
	}
	emit rowsLoaded(rowCount);

	return { SQLTableModel::DataRefreshResult::Refreshed, SQLTableModel::DataRefreshType::Full, sqlData };
}

IzSQLUtilities::SQLTableModel::LoadedData IzSQLUtilities::SQLTableModel::partialDataRefresh(const QString& sqlQuery, const QVariantMap& sqlParameters, const QList<int>& rows)
{
	qDebug() << "implement me :[";
	return { SQLTableModel::DataRefreshResult::Refreshed, SQLTableModel::DataRefreshType::Full, std::shared_ptr<LoadedSQLData>() };
}

QVariantMap IzSQLUtilities::SQLTableModel::sqlQueryParameters() const
{
	return m_sqlQueryParameters;
}

void IzSQLUtilities::SQLTableModel::setSqlQueryParameters(const QVariantMap& sqlQueryParameters)
{
	if (m_sqlQueryParameters != sqlQueryParameters) {
		m_sqlQueryParameters = sqlQueryParameters;
		emit sqlQueryParametersChanged();
	}
}

void IzSQLUtilities::SQLTableModel::refreshData(const QString& sqlQuery, const QVariantMap& sqlParameters, const QList<int>& rows)
{
	if (m_isRefreshing) {
		qCritical() << "Data refresh is not possible - model is still loading data.";
		return;
	}
	setQueryIsValid(validateSqlQuery(sqlQuery, sqlParameters));
	if (!queryIsValid()) {
		qCritical() << "Sql query or its parameters are invalid.";
		return;
	}
	setSqlQuery(sqlQuery);
	setSqlQueryParameters(sqlParameters);

	setIsRefreshing(true);

	emit refreshStarted();

	if (rows.isEmpty()) {
		QFuture<LoadedData> refreshFuture = QtConcurrent::run([this, query = m_sqlQuery, parameters = m_sqlQueryParameters]()->LoadedData {
			return this->fullDataRefresh(normalizeSqlQuery(query, parameters), parameters);
		});
		m_refreshFutureWatcher->setFuture(refreshFuture);
	} else {
		QFuture<LoadedData> refreshFuture = QtConcurrent::run([this, query = m_sqlQuery, parameters = m_sqlQueryParameters, rows = rows]()->LoadedData {
			return this->partialDataRefresh(normalizeSqlQuery(query, parameters), parameters, rows);
		});
		m_refreshFutureWatcher->setFuture(refreshFuture);
	}
}

void IzSQLUtilities::SQLTableModel::refreshData()
{
	if (m_isRefreshing) {
		qCritical() << "Data refresh is not possible - model is still loading data.";
		return;
	}
	setQueryIsValid(validateSqlQuery(m_sqlQuery, m_sqlQueryParameters));
	if (!queryIsValid()) {
		qCritical() << "Preconstructed query is invalid. Aborting refresh.";
		return;
	}

	setIsRefreshing(true);

	emit refreshStarted();

	QFuture<LoadedData> refreshFuture = QtConcurrent::run([this, query = m_sqlQuery, parameters = m_sqlQueryParameters]()->LoadedData {
		return this->fullDataRefresh(normalizeSqlQuery(query, parameters), parameters);
	});
	m_refreshFutureWatcher->setFuture(refreshFuture);
}

bool IzSQLUtilities::SQLTableModel::isRefreshing() const
{
	return m_isRefreshing;
}

QString IzSQLUtilities::SQLTableModel::sqlQuery() const
{
	return m_sqlQuery;
}

void IzSQLUtilities::SQLTableModel::setSqlQuery(const QString& sqlQuery)
{
	if (m_sqlQuery != sqlQuery) {
		m_sqlQuery = sqlQuery;
		emit sqlQueryChanged();
	}
}
