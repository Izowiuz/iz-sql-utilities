#include "IzSQLUtilities/AbstractSQLModel.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QtConcurrent>

#include "IzSQLUtilities/SQLConnector.h"
#include "IzSQLUtilities/SQLErrorEvent.h"

#include "LoadedSQLData.h"

IzSQLUtilities::AbstractSQLModel::AbstractSQLModel(QObject* parent)
	: IzModels::AbstractItemModel(parent)
	, m_refreshFutureWatcher(new QFutureWatcher<LoadedData>(this))
{
	// watchers setup
	connect(m_refreshFutureWatcher, &QFutureWatcher<LoadedData>::finished, this, &AbstractSQLModel::parseSQLData);
}

QVariant IzSQLUtilities::AbstractSQLModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	Q_UNUSED(role)
	if (orientation == Qt::Horizontal) {
		return m_columnNameColumnAliasMap.contains(columnNameFromIndex(section)) ? m_columnNameColumnAliasMap[columnNameFromIndex(section)] : m_indexColumnMap[section];
	}

	return section;
}

bool IzSQLUtilities::AbstractSQLModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
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

QModelIndex IzSQLUtilities::AbstractSQLModel::index(int row, int column, const QModelIndex& parent) const
{
	if (hasIndex(row, column, parent)) {
		return createIndex(row, column);
	}

	return {};
}

QModelIndex IzSQLUtilities::AbstractSQLModel::parent(const QModelIndex& index) const
{
	Q_UNUSED(index)
	return {};
}

int IzSQLUtilities::AbstractSQLModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return static_cast<int>(m_data.size());
}

int IzSQLUtilities::AbstractSQLModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return m_indexColumnMap.size();
}

bool IzSQLUtilities::AbstractSQLModel::hasChildren(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return {};
}

bool IzSQLUtilities::AbstractSQLModel::canFetchMore(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	return false;
}

void IzSQLUtilities::AbstractSQLModel::fetchMore(const QModelIndex& parent){
	Q_UNUSED(parent)
}

QString IzSQLUtilities::AbstractSQLModel::columnNameFromIndex(int index) const
{
	return m_indexColumnMap.value(index);
}

int IzSQLUtilities::AbstractSQLModel::indexFromColumnName(const QString& column) const
{
	// WARNING: tu nastąpiły bardzo podejrzane zmiany
	if (m_columnAliasColumnNameMap.contains(column)) {
		return m_columnIndexMap.value(m_columnAliasColumnNameMap.value(column).toString(), -1);
	}

	return m_columnIndexMap.value(column, -1);
}

bool IzSQLUtilities::AbstractSQLModel::validateSqlQuery(const QString& sqlQuery, const QVariantMap& sqlParameters, bool silent)
{
	bool res{ true };
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
				if (!silent) {
					qCritical() << "Parameter" << str << "not found in passed parameters.";
				}
				res = false;
			}
			parametersFound++;
			delimiterStart = -1;
			delimiterEnd   = -1;
		}
	}

	if (delimiterStart != -1 && delimiterEnd == -1) {
		if (!silent) {
			qCritical() << "Unterminated delimiter. Delimiter start position:" << delimiterStart;
		}
		res = false;
	}

	if (parametersFound != sqlParameters.size()) {
		if (!silent) {
			qCritical() << "Number of parameters in query are not equal to passed parameters.";
		}
		res = false;
	}

	if (m_queryIsValid != res) {
		m_queryIsValid = res;
		emit queryIsValidChanged();
	}

	if (res) {
		emit validQuerySet();
	}

	return res;
}

QString IzSQLUtilities::AbstractSQLModel::normalizeSqlQuery(const QString& sqlQuery, const QVariantMap& sqlParameters) const
{
	QString tmp = sqlQuery;
	QMapIterator<QString, QVariant> it(sqlParameters);
	while (it.hasNext()) {
		it.next();
		tmp.replace(QStringLiteral("'") + it.key() + QStringLiteral("'"), it.key());
	}

	return tmp;
}

bool IzSQLUtilities::AbstractSQLModel::queryIsValid() const
{
	return m_queryIsValid;
}

QVariantMap IzSQLUtilities::AbstractSQLModel::columnNameColumnAliasMap() const
{
	return m_columnNameColumnAliasMap;
}

void IzSQLUtilities::AbstractSQLModel::setColumnNameColumnAliasMap(const QVariantMap& columnNameColumnAliasMap)
{
	m_columnNameColumnAliasMap = columnNameColumnAliasMap;
	m_columnAliasColumnNameMap.clear();

	QMapIterator<QString, QVariant> it(m_columnNameColumnAliasMap);
	while (it.hasNext()) {
		it.next();
		m_columnAliasColumnNameMap.insert(it.value().toString(), it.key());
	}
}

QMetaType::Type IzSQLUtilities::AbstractSQLModel::columnDataType(int index) const
{
	if (index >= 0 && static_cast<std::size_t>(index) < m_sqlDataTypes.size()) {
		return m_sqlDataTypes[static_cast<std::size_t>(index)];
	}

	return QMetaType::UnknownType;
}

int IzSQLUtilities::AbstractSQLModel::roleNameToColumn(const QString& roleName)
{
	return m_columnIndexMap.value(roleName, -1);
}

const std::vector<std::unique_ptr<IzSQLUtilities::SQLRow>>& IzSQLUtilities::AbstractSQLModel::internalData() const
{
	return m_data;
}

const QMap<int, QString>& IzSQLUtilities::AbstractSQLModel::indexColumnMap() const
{
	return m_indexColumnMap;
}

const QHash<QString, int>& IzSQLUtilities::AbstractSQLModel::columnIndexMap() const
{
	return m_columnIndexMap;
}

void IzSQLUtilities::AbstractSQLModel::additionalDataParsing(bool dataRefreshSucceeded){
	Q_UNUSED(dataRefreshSucceeded)
}

std::vector<std::unique_ptr<IzSQLUtilities::SQLRow>>& IzSQLUtilities::AbstractSQLModel::internalData()
{
	return m_data;
}

void IzSQLUtilities::AbstractSQLModel::parseSQLData()
{
	if (std::get<0>(m_refreshFutureWatcher->result()) == AbstractSQLModel::DataRefreshResult::Refreshed) {
		beginResetModel();

		m_data.swap(std::get<2>(m_refreshFutureWatcher->result())->sqlData());
		m_sqlDataTypes.swap(std::get<2>(m_refreshFutureWatcher->result())->sqlDataTypes());
		m_columnIndexMap = std::get<2>(m_refreshFutureWatcher->result())->columnIndexMap();
		m_indexColumnMap = std::get<2>(m_refreshFutureWatcher->result())->indexColumnMap();

		additionalDataParsing(true);
		endResetModel();

		emit dataRefreshEnded(true);
	} else {
		beginResetModel();

		m_data.clear();
		m_sqlDataTypes.clear();
		m_columnIndexMap.clear();
		m_indexColumnMap.clear();

		additionalDataParsing(false);
		endResetModel();

		emit dataRefreshEnded(false);
	}
}

IzSQLUtilities::AbstractSQLModel::LoadedData IzSQLUtilities::AbstractSQLModel::fullDataRefresh(const QString& sqlQuery, const QVariantMap& sqlParameters)
{
	// database connect
	SqlConnector db(m_databaseType, m_connectionParameters);
	if (!db.getConnection().isOpen()) {
		SQLErrorEvent::postSQLError(db.lastError());
		return { AbstractSQLModel::DataRefreshResult::DatabaseError, AbstractSQLModel::DataRefreshType::Full, std::shared_ptr<LoadedSQLData>() };
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

	emit rowsLoaded(0);

	// query exec
	emit sqlQueryStarted();
	if (!query.exec()) {
		qWarning() << query.lastError();
		SQLErrorEvent::postSQLError(query.lastError());
		return { AbstractSQLModel::DataRefreshResult::QueryError, AbstractSQLModel::DataRefreshType::Full, std::shared_ptr<LoadedSQLData>() };
	}
	emit sqlQueryReturned();

	m_newQuery  = (m_lastQuery != query.lastQuery());
	m_lastQuery = query.lastQuery();

	// additional data
	int rowCount     = 0;
	int columnsCount = query.record().count();

	QHash<QString, int> columnIndexMap;
	columnIndexMap.reserve(columnsCount);

	QMap<int, QString> indexColumnMap;

	std::vector<QMetaType::Type> dataTypes;
	dataTypes.reserve(static_cast<std::size_t>(columnsCount));

	for (int i = 0; i < columnsCount; i++) {
		dataTypes.emplace_back(static_cast<QMetaType::Type>(query.record().value(i).type()));
		columnIndexMap.insert(query.record().fieldName(i), i);
		indexColumnMap.insert(i, query.record().fieldName(i));
	}

	auto sqlData = std::make_shared<LoadedSQLData>();

	sqlData->setSqlDataTypes(dataTypes);
	sqlData->setColumnIndexMap(columnIndexMap);
	sqlData->setIndexColumnMap(indexColumnMap);

	// query data
	while (query.next()) {
		if (rowCount % 100 == 0) {
			emit rowsLoaded(rowCount);
		}

		auto row = std::make_unique<SQLRow>(static_cast<std::size_t>(columnsCount));
		for (int i = 0; i < columnsCount; ++i) {
			row->addColumnValue(query.value(i));
		}

		sqlData->addRow(std::move(row));
		rowCount++;
	}
	emit rowsLoaded(rowCount);

	return { AbstractSQLModel::DataRefreshResult::Refreshed, AbstractSQLModel::DataRefreshType::Full, sqlData };
}

IzSQLUtilities::AbstractSQLModel::LoadedData IzSQLUtilities::AbstractSQLModel::partialDataRefresh(const QString& sqlQuery, const QVariantMap& sqlParameters, const QList<int>& rows)
{
	Q_UNUSED(sqlQuery)
	Q_UNUSED(sqlParameters)
	Q_UNUSED(rows)

	qDebug() << "implement me :[";
	return { AbstractSQLModel::DataRefreshResult::Refreshed, AbstractSQLModel::DataRefreshType::Full, std::shared_ptr<LoadedSQLData>() };
}

QString IzSQLUtilities::AbstractSQLModel::databaseName() const
{
	return m_databaseName;
}

void IzSQLUtilities::AbstractSQLModel::setDatabaseName(const QString& databaseName)
{
	if (databaseName == QStringLiteral("MSSQL")) {
		setDatabaseType(IzSQLUtilities::DatabaseType::MSSQL);
	} else if (databaseName == QStringLiteral("PSQL")) {
		setDatabaseType(IzSQLUtilities::DatabaseType::PSQL);
	} else if (databaseName == QStringLiteral("SQLITE")) {
		setDatabaseType(IzSQLUtilities::DatabaseType::SQLITE);
	} else {
		qWarning() << "Got invalid database name:" << databaseName;
	}
}

IzSQLUtilities::DatabaseType IzSQLUtilities::AbstractSQLModel::databaseType() const
{
	return m_databaseType;
}

void IzSQLUtilities::AbstractSQLModel::setDatabaseType(const IzSQLUtilities::DatabaseType& databaseType)
{
	if (m_databaseType != databaseType) {
		m_databaseType = databaseType;

		switch (m_databaseType) {
		case DatabaseType::MSSQL:
			m_databaseName = QStringLiteral("MSSQL");
			break;
		case DatabaseType::SQLITE:
			m_databaseName = QStringLiteral("SQLITE");
			break;
		case DatabaseType::PSQL:
			m_databaseName = QStringLiteral("PSQL");
			break;
		}

		emit databaseTypeChanged();
	}
}

QVariantMap IzSQLUtilities::AbstractSQLModel::connectionParameters() const
{
	return m_connectionParameters;
}

void IzSQLUtilities::AbstractSQLModel::setConnectionParameters(const QVariantMap& connectionParameters)
{
	if (m_connectionParameters != connectionParameters) {
		m_connectionParameters = connectionParameters;
		emit connectionParametersChanged();
	}
}

QVariantMap IzSQLUtilities::AbstractSQLModel::sqlQueryParameters() const
{
	return m_sqlQueryParameters;
}

void IzSQLUtilities::AbstractSQLModel::setSqlQueryParameters(const QVariantMap& sqlQueryParameters)
{
	if (m_sqlQueryParameters != sqlQueryParameters) {
		m_sqlQueryParameters = sqlQueryParameters;
		emit sqlQueryParametersChanged();
		if (!m_sqlQuery.isEmpty()) {
			validateSqlQuery(m_sqlQuery, m_sqlQueryParameters, true);
		}
	}
}

void IzSQLUtilities::AbstractSQLModel::clearData()
{
	emit isRefreshingDataChanged();
	emit dataRefreshStarted();

	beginResetModel();
	m_data.clear();
	m_columnIndexMap.clear();
	m_indexColumnMap.clear();
	endResetModel();

	emit dataRefreshEnded(true);
	emit isRefreshingDataChanged();
}

void IzSQLUtilities::AbstractSQLModel::clearQueryData()
{
	m_sqlQuery.clear();
	m_sqlQueryParameters.clear();
	m_queryIsValid = false;
	emit queryIsValidChanged();
}

void IzSQLUtilities::AbstractSQLModel::refreshData(const QString& sqlQuery, const QVariantMap& sqlParameters, const QList<int>& rows)
{
	emit aboutToRefreshData();

	if (isRefreshingData()) {
		qCritical() << "Data refresh is not possible - model is still loading data.";
		return;
	}

	validateSqlQuery(sqlQuery, sqlParameters);
	if (!queryIsValid()) {
		qCritical() << "Sql query or its parameters are invalid.";
		return;
	}
	setSqlQuery(sqlQuery);
	setSqlQueryParameters(sqlParameters);

	emit dataRefreshStarted();

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

void IzSQLUtilities::AbstractSQLModel::refreshData()
{
	if (isRefreshingData()) {
		qCritical() << "Data refresh is not possible - model is still loading data.";
		return;
	}

	if (!queryIsValid()) {
		qCritical() << "Preconstructed query is invalid. Aborting refresh.";
		return;
	}

	emit dataRefreshStarted();

	QFuture<LoadedData> refreshFuture = QtConcurrent::run([this, query = m_sqlQuery, parameters = m_sqlQueryParameters]()->LoadedData {
		return this->fullDataRefresh(normalizeSqlQuery(query, parameters), parameters);
	});
	m_refreshFutureWatcher->setFuture(refreshFuture);
}

QString IzSQLUtilities::AbstractSQLModel::sqlQuery() const
{
	return m_sqlQuery;
}

void IzSQLUtilities::AbstractSQLModel::setSqlQuery(const QString& sqlQuery)
{
	if (m_sqlQuery != sqlQuery) {
		m_sqlQuery = sqlQuery;
		validateSqlQuery(m_sqlQuery, m_sqlQueryParameters, true);
		emit sqlQueryChanged();
	}
}

void IzSQLUtilities::AbstractSQLModel::addQueryParameter(const QString& parameter, const QVariant& value)
{
	m_sqlQueryParameters.insert(parameter, value);
	validateSqlQuery(m_sqlQuery, m_sqlQueryParameters, true);
}

bool IzSQLUtilities::AbstractSQLModel::addRow(const QVariantMap& data, bool defaultInitialize, const QStringList& uniqueColumnValues)
{
	// we have to check for:
	//	all columns
	//	valid type for column
	//	data uniqueness - if requested

	// temporary row
	auto row = std::make_unique<SQLRow>(columnCount());

	QMapIterator<int, QString> it(m_indexColumnMap);
	while (it.hasNext()) {
		it.next();

		QMetaType::Type expectedDataType = columnDataType(it.key());

		// valid column
		if (data.contains(it.value())) {
			QMetaType::Type dataType = static_cast<QMetaType::Type>(data.value(it.value()).type());

			// valid data type
			if (dataType != expectedDataType) {
				qCritical() << "Cannot add new data row, column:" << it.key() << " - " << it.value() << "has invalid data type. Got:" << QMetaType::typeName(dataType) << "expected:" << QMetaType::typeName(expectedDataType);
				return false;
			}

			// all is ok - add column value
			row->addColumnValue(data.value(it.value()));
		} else {
			// we have not enabled initialization by default value
			if (!defaultInitialize) {
				qCritical() << "Cannot add new data row, column:" << it.key() << " - " << it.value() << "has not been given a value and default-initialization is disabled.";
				return false;
			}

			// we have enabled initialization by default value
			row->addColumnValue(QVariant(static_cast<QVariant::Type>(expectedDataType)));
		}
	}

	// uniqueColumnValues specified - construct map for search purposes
	if (!uniqueColumnValues.isEmpty()) {
		QVariantMap valuesToSearchFor;

		for (const auto& column : qAsConst(uniqueColumnValues)) {
			valuesToSearchFor.insert(column, data.value(column));
		}

		auto dataIndex = findRow(valuesToSearchFor);
		if (dataIndex != -1) {
			qCritical() << "Cannot add data row - found duplicated column values. Columns requested to search for:" << uniqueColumnValues;
			emit duplicateFound();
			return false;
		}
	}

	// actually add new data
	beginInsertRows({}, rowCount(), rowCount());
	m_data.emplace_back(std::move(row));
	endInsertRows();

	return true;
}

bool IzSQLUtilities::AbstractSQLModel::removeRow(int index)
{
	if (!indexIsValid(index)) {
		qCritical() << "Cannot remove data row, index:" << index << "is invalid.";
		return false;
	}

	// remove data
	beginRemoveRows({}, index, index);
	m_data.erase(m_data.begin() + index);
	endRemoveRows();

	return false;
}

bool IzSQLUtilities::AbstractSQLModel::executedNewQuery() const
{
	return m_newQuery;
}

int IzSQLUtilities::AbstractSQLModel::findRow(const QVariantMap& columnValues) const
{
	auto pos = std::find_if(m_data.begin(), m_data.end(), [this, &columnValues](const auto& row) -> bool {
		int hits{ 0 };
		QMapIterator<QString, QVariant> it(columnValues);
		while (it.hasNext()) {
			it.next();
			if (row->columnValue(indexFromColumnName(it.key())) == it.value()) {
				hits++;
			}
		}
		return hits == columnValues.size();
	});

	return pos == m_data.end() ? -1 : static_cast<int>(std::distance(m_data.begin(), pos));
}
