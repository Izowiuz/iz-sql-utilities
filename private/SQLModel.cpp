#include "IzSQLUtilities/SQLModel.h"

#include <algorithm>

#include <QDebug>
#include <QMetaObject>
#include <QMetaProperty>

#include "IzSQLUtilities/SQLDataContainer.h"
#include "IzSQLUtilities/SQLQueryBuilder.h"
#include "SQLData.h"
#include "SQLDataLoader.h"

IzSQLUtilities::SQLModel::SQLModel(QObject* parent)
	: QAbstractItemModel(parent)
	, m_dataLoader(new SQLDataLoader(this))
	, m_queryBuilder(new SQLQueryBuilder(this))
{
	// data loader connects
	connect(m_dataLoader, &SQLDataLoader::workCompleted, this, &IzSQLUtilities::SQLModel::processData);
	connect(m_dataLoader, &SQLDataLoader::loadedCount, this, &IzSQLUtilities::SQLModel::rowsLoaded);
	connect(this, &IzSQLUtilities::SQLModel::abortRefresh, m_dataLoader, &SQLDataLoader::abortOperation, Qt::DirectConnection);

	qRegisterMetaType<IzSQLUtilities::SQLModel*>("IzSQLUtilities::SQLModel*");
}

int IzSQLUtilities::SQLModel::rowCount(const QModelIndex& index) const
{
	Q_UNUSED(index)
	return m_modelData.size();
}

int IzSQLUtilities::SQLModel::columnCount(const QModelIndex& index) const
{
	Q_UNUSED(index)
	return m_indexColumnMap.size();
}

QVariant IzSQLUtilities::SQLModel::data(const QModelIndex& index, int role) const
{
	if (role >= Qt::UserRole) {
		switch (role) {
		case Qt::UserRole:
			return isAdded(index.row());
		case Qt::UserRole + 1:
			return toBeRemoved(index.row());
		case Qt::UserRole + 2:
			return rowIsDirty(index.row());
		default:
			return m_modelData.at(index.row())->fieldValue(role - Qt::UserRole - 3);
		}
	}
	if (role == Qt::DisplayRole) {
		return m_modelData.at(index.row())->fieldValue(index.column());
	}
	return {};
}

QModelIndex IzSQLUtilities::SQLModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent)) {
		return {};
	}
	return createIndex(row, column);
}

QModelIndex IzSQLUtilities::SQLModel::parent(const QModelIndex& index) const
{
	Q_UNUSED(index)
	return {};
}

QHash<int, QByteArray> IzSQLUtilities::SQLModel::roleNames() const
{
	if (m_indexColumnMap.empty()) {
		return {};
	}
	int role = Qt::UserRole;
	m_cachedRoleNames.clear();
	m_cachedRoleNames[role]     = QByteArrayLiteral("__isAdded__");
	m_cachedRoleNames[role + 1] = QByteArrayLiteral("__toBeRemoved__");
	m_cachedRoleNames[role + 2] = QByteArrayLiteral("__isDirty__");
	for (int i = 0; i < m_indexColumnMap.size(); ++i) {
		m_cachedRoleNames[role + (i + 3)] = m_indexColumnMap.value(i).toUtf8();
	}
	return m_cachedRoleNames;
}

void IzSQLUtilities::SQLModel::refreshData(const QString& sqlQuery, bool reportProgress, bool asynchronousLoad)
{
	if (m_isLoadingData) {
		if (m_allowContinuousRefreshes) {
			qInfo() << "Restart requested.";
			m_dataLoader->restartOperation(sqlQuery);
		} else {
			qWarning() << "Data refresh not possible. Model is still working.";
			qWarning() << "Requested query:" << (sqlQuery.isEmpty() ? getQueryBuilder()->getQuery() : sqlQuery);
			return;
		}
	}
	QString query;
	m_isLoadingData = true;
	emit refreshStarted();

	// data model clear started
	beginResetModel();
	m_columnIndexMap.clear();
	m_indexColumnMap.clear();
	m_modelData.clear();
	clearDirtyRows();
	endResetModel();
	emit dataCleared();
	// data model clear endend

	m_loadStatus = ModelLoadStatus::LOADING;
	// null QString was passed - refresh query using query builder
	if (sqlQuery.isNull()) {
		if (!this->getQueryBuilder()->queryIsValid()) {
			qCritical() << "Query builder reported invalid query. Refresh aborted.";
			m_isLoadingData = false;
			emit refreshFailed();
			m_loadStatus = ModelLoadStatus::QUERY_ERROR;
			return;
		}
		query = getQueryBuilder()->getQuery();
	} else {
		query = sqlQuery;
	}
	m_dataLoader->loadData(query, asynchronousLoad, false, reportProgress, m_emitOnAbort);
}

void IzSQLUtilities::SQLModel::refreshRows(const QList<int>& rows, bool reportProgress)
{
	if (m_isLoadingData) {
		qWarning() << "Data refresh not possible. Model is still working.";
		return;
	}
	if (m_modelData.isEmpty()) {
		qWarning() << "Nothing to refresh. Model is empty.";
		return;
	}
	if (rows.size() == m_modelData.size()) {
		refreshData(QString(), true, true);
		qInfo() << "Rows to refresh == model size. Switching to full refresh.";
		return;
	}
	if (m_identityColumn.isEmpty() || m_identityTable.isEmpty()) {
		qCritical() << "Identity values are invalid. Partial refresh is not possible.";
		return;
	}
	if (!roleNames().values().contains(m_identityColumn.toLocal8Bit())) {
		qCritical() << "Identity value:" << m_identityColumn << "not found in element properties. Partial refresh is not possible.";
		return;
	}
	QPair<QString, QMap<int, QVariant>> elementsToRefresh;
	elementsToRefresh.first = m_identityColumn;
	for (const auto index : qAsConst(rows)) {
		if (hasIndex(index, 0, QModelIndex())) {
			elementsToRefresh.second.insert(index, m_modelData.at(index)->fieldValue(getColumnIndex(m_identityColumn)));
		} else {
			qCritical() << "Invalid index for row:" << index << "requested.";
			return;
		}
	}
	if (!this->getQueryBuilder()->queryIsValid()) {
		qCritical() << "Query builder reported invalid query. Partial refresh aborted.";
		m_isLoadingData = false;
		emit refreshFailed();
		m_loadStatus = ModelLoadStatus::QUERY_ERROR;
		return;
	}
	QString partialQuery = getQueryBuilder()->getPartialQuery(m_identityColumn, m_identityTable, elementsToRefresh.second.values());
	if (partialQuery.isEmpty()) {
		qCritical() << "Query builder returned empty query. Partial refresh will not be possible.";
		return;
	}
	m_isLoadingData = true;
	emit refreshStarted(true);
	m_loadStatus = ModelLoadStatus::LOADING;
	m_dataLoader->loadData(partialQuery, true, true, reportProgress, m_emitOnAbort, elementsToRefresh);
}

bool IzSQLUtilities::SQLModel::processData(QSharedPointer<IzSQLUtilities::SQLData> data)
{
	m_loadStatus   = data->loadStatus();
	bool isPartial = data->getPartialRefresh();
	if (data->loadStatus() == ModelLoadStatus::QUERY_ERROR || data->loadStatus() == ModelLoadStatus::SQL_ERROR) {
		qCritical() << "Model refresh failed.";
		m_isLoadingData = false;
		emit refreshFailed();
		return false;
	}
	//	if (data->loadStatus() == ModelLoadStatus::ABORTED) {
	//		qInfo() << "Aborted from refresh operation.");
	//		emit refreshAborted();
	//		m_isLoadingData = false;
	//		emit workStatusChanged();
	//		return true;
	//	}
	if (!isPartial) {
		beginResetModel();
		m_columnIndexMap = data->getColumnIndexMap();
		m_indexColumnMap = data->getIndexColumnMap();
		m_sqlColumnNames = data->getSQLColumnNames();
		m_modelData      = data->sqlData();
		endResetModel();
		qInfo() << "Data from loader appended. Model size:" << m_modelData.count();
	} else {
		QMap<int, QVariant> refreshed = data->getRefreshedElements().second;
		QVector<int> removed          = data->getRemovedElements();
		for (const auto& element : data->sqlData()) {
			int key = refreshed.key(element->fieldValue(getColumnIndex(m_identityColumn)));
			m_dirtyRows.remove(m_modelData.at(key));
			m_modelData.replace(key, element);
			emit dataChanged(index(key, 0), index(key, 0));
		}
		std::stable_sort(removed.begin(), removed.end());
		for (auto iter = removed.rbegin(); iter != removed.rend(); ++iter) {
			m_dirtyRows.remove(m_modelData[*iter]);
			removeRow(*iter);
		}
		emit rowsLoaded(dataSize());
		emit partialRefreshOutcome(refreshed.size(), removed.size());
		qInfo() << "Partial refresh completed. Reloaded:" << refreshed.size() << ", removed:" << removed.size();
	}
	if (data->loadStatus() == ModelLoadStatus::ABORTED) {
		qInfo() << "Aborted from refresh operation.";
	}
	m_isLoadingData = false;
	emit refreshCompleted(isPartial);
	return true;
}

void IzSQLUtilities::SQLModel::updateRowState(QSharedPointer<SQLDataContainer> row)
{
	if (m_dirtyRows.contains(row) && !row->isDirty()) {
		m_dirtyRows.remove(row);
		emit modelStateChanged(isDirty());
	} else if (!m_dirtyRows.contains(row) && row->isDirty()) {
		m_dirtyRows.insert(row);
		emit modelStateChanged(isDirty());
	}
}

void IzSQLUtilities::SQLModel::clearDirtyRows()
{
	m_dirtyRows.clear();
	emit modelStateChanged(false);
}

IzSQLUtilities::DatabaseType IzSQLUtilities::SQLModel::getDatabaseType() const
{
	return m_dataLoader->databaseType();
}

void IzSQLUtilities::SQLModel::setDatabaseType(IzSQLUtilities::DatabaseType databaseType)
{
	m_dataLoader->setDatabaseType(databaseType);
}

QVariantMap IzSQLUtilities::SQLModel::getDatabaseParameters() const
{
	return m_dataLoader->databaseParameters();
}

void IzSQLUtilities::SQLModel::setDatabaseParameters(const QVariantMap& databaseParameters)
{
	m_dataLoader->setDatabaseParameters(databaseParameters);
}

QStringList IzSQLUtilities::SQLModel::getFieldsToReplace() const
{
	return m_fieldsToReplace;
}

void IzSQLUtilities::SQLModel::setFieldsToReplace(const QStringList& fieldsToReplace)
{
	m_fieldsToReplace = fieldsToReplace;
}

bool IzSQLUtilities::SQLModel::getAllowContinuousRefreshes() const
{
	return m_allowContinuousRefreshes;
}

void IzSQLUtilities::SQLModel::setAllowContinuousRefreshes(bool allowContinuousRefreshes)
{
	m_allowContinuousRefreshes = allowContinuousRefreshes;
}

int IzSQLUtilities::SQLModel::getColumnIndex(const QString& columnName) const
{
	return m_columnIndexMap.value(columnName, -1);
}

QVariantMap IzSQLUtilities::SQLModel::getUniqueFieldValues(const QStringList& columnNames) const
{
	QVariantMap uniqueValues;
	QVariantList tmpValues;
	for (auto const& column : qAsConst(columnNames)) {
		for (int i = 0; i < rowCount(); ++i) {
			if (!tmpValues.contains(getFieldValue(i, column))) {
				tmpValues.append(getFieldValue(i, column));
			}
		}
		uniqueValues.insert(column, tmpValues);
		tmpValues.clear();
	}
	return uniqueValues;
}

QList<QVariantMap> IzSQLUtilities::SQLModel::getChangedRowsData() const
{
	if (!isDirty()) {
		return {};
	}
	QList<QVariantMap> res;
	for (auto i = 0; i < rowCount(); ++i) {
		if (rowIsDirty(i)) {
			res.append(getRowData(i));
		}
	}
	return res;
}

QVector<QSharedPointer<IzSQLUtilities::SQLDataContainer>> IzSQLUtilities::SQLModel::cloneData() const
{
	return m_modelData;
}

bool IzSQLUtilities::SQLModel::getReplaceFieldsOnAdd() const
{
	return m_replaceFieldsOnAdd;
}

void IzSQLUtilities::SQLModel::setReplaceFieldsOnAdd(bool replaceFieldsOnAdd)
{
	m_replaceFieldsOnAdd = replaceFieldsOnAdd;
}

QVariantMap IzSQLUtilities::SQLModel::getDefaultFieldValues() const
{
	return m_defaultDatafieldValues;
}

void IzSQLUtilities::SQLModel::setDefaultFieldValues(const QVariantMap& defaultFieldValues)
{
	m_defaultDatafieldValues = defaultFieldValues;
}

QString IzSQLUtilities::SQLModel::getIdentityTable() const
{
	return m_identityTable;
}

void IzSQLUtilities::SQLModel::setIdentityTable(const QString& tableName)
{
	m_identityTable = tableName;
}

QString IzSQLUtilities::SQLModel::getIdentityColumn() const
{
	return m_identityColumn;
}

void IzSQLUtilities::SQLModel::setIdentityColumn(const QString& columnName)
{
	m_identityColumn = columnName;
}

void IzSQLUtilities::SQLModel::clearData()
{
	if (m_isLoadingData) {
		qWarning() << "Not possible to clear data - model is currently refreshing.";
		return;
	}
	if (m_modelData.empty()) {
		qInfo() << "Nothing to clear. Model is empty.";
		return;
	}
	beginResetModel();
	m_columnIndexMap.clear();
	m_indexColumnMap.clear();
	m_modelData.clear();
	clearDirtyRows();
	endResetModel();
	emit dataCleared();
}

int IzSQLUtilities::SQLModel::dataSize() const
{
	return m_modelData.size();
}

IzSQLUtilities::SQLQueryBuilder* IzSQLUtilities::SQLModel::getQueryBuilder() const
{
	return m_queryBuilder;
}

void IzSQLUtilities::SQLModel::requestAbort()
{
	if (!isInLoadProcess()) {
		qWarning() << "Model is currently not loading data.";
		return;
	}
	emit abortRefresh();
}

IzSQLUtilities::ModelLoadStatus IzSQLUtilities::SQLModel::getLoadStatus() const
{
	return m_loadStatus;
}

bool IzSQLUtilities::SQLModel::setFieldValue(int row, const QString& columnName, const QVariant& fieldValue)
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	bool result = m_modelData.at(row)->setFieldValue(getColumnIndex(columnName), fieldValue);
	if (result) {
		updateRowState(m_modelData.at(row));
		emit dataAboutToBeChanged(row, columnName, fieldValue);
		emit dataChanged(index(row, 0), index(row, 0), QVector<int>{ m_cachedRoleNames.key(columnName.toUtf8()), m_cachedRoleNames.key(QByteArrayLiteral("__isDirty__")) });
		return true;
	}
	return false;
}

bool IzSQLUtilities::SQLModel::setFieldValue(int row, const char* columnName, const QVariant& fieldValue)
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	bool result = m_modelData.at(row)->setFieldValue(getColumnIndex(columnName), fieldValue);
	if (result) {
		updateRowState(m_modelData.at(row));
		emit dataAboutToBeChanged(row, columnName, fieldValue);
		emit dataChanged(index(row, 0), index(row, 0), QVector<int>{ m_cachedRoleNames.key(columnName), m_cachedRoleNames.key(QByteArrayLiteral("__isDirty__")) });
		return true;
	}
	return false;
}

QVariant IzSQLUtilities::SQLModel::getFieldValue(int row, const QString& columnName) const
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return {};
	}
	const auto idx = getColumnIndex(columnName);
	if (idx == -1) {
		qCritical() << "Invalid column requested. Column name:" << columnName;
		return {};
	}
	return m_modelData.at(row)->fieldValue(idx);
}

QVariant IzSQLUtilities::SQLModel::getFieldValue(int row, const char* columnName) const
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return {};
	}
	return m_modelData.at(row)->fieldValue(getColumnIndex(columnName));
}

QVariantMap IzSQLUtilities::SQLModel::getRowData(int row) const
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return {};
	}
	QVariantMap properties;
	for (auto iter = m_columnIndexMap.cbegin(); iter != m_columnIndexMap.cend(); ++iter) {
		properties.insert(iter.key(), getFieldValue(row, iter.key()));
	}
	return properties;
}

void IzSQLUtilities::SQLModel::setRowsData(const QVariantMap& fieldValues)
{
	for (auto i = 0; i < m_modelData.size(); ++i) {
		QMapIterator<QString, QVariant> iter(fieldValues);
		while (iter.hasNext()) {
			iter.next();
			setFieldValue(i, iter.key(), iter.value());
		}
	}
}

bool IzSQLUtilities::SQLModel::removeRow(int row)
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	beginRemoveRows(QModelIndex(), row, row);
	bool lastModelState = isDirty();
	m_dirtyRows.remove(m_modelData.at(row));
	if (lastModelState != isDirty()) {
		emit modelStateChanged(isDirty());
	}
	m_modelData.removeAt(row);
	endRemoveRows();
	return true;
}

bool IzSQLUtilities::SQLModel::fieldIsDirty(int row, const QString& columnName) const
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	return m_modelData.at(row)->fieldIsDirty(getColumnIndex(columnName));
}

bool IzSQLUtilities::SQLModel::cleanField(int row, const QString& columnName)
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	bool result = m_modelData.at(row)->cleanField(getColumnIndex(columnName));
	if (result) {
		updateRowState(m_modelData.at(row));
		emit dataChanged(index(row, 0), index(row, 0), QVector<int>{ m_cachedRoleNames.key(columnName.toUtf8()) });
		return true;
	}
	return false;
}

bool IzSQLUtilities::SQLModel::rowIsDirty(int row) const
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	return m_modelData.at(row)->isDirty();
}

bool IzSQLUtilities::SQLModel::cleanRow(int row)
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	if (m_modelData.at(row)->cleanContainer()) {
		updateRowState(m_modelData.at(row));
		emit dataChanged(index(row, 0), index(row, 0));
		return true;
	}
	return false;
}

bool IzSQLUtilities::SQLModel::isDirty() const
{
	return !m_dirtyRows.empty();
}

int IzSQLUtilities::SQLModel::changedRowsCount() const
{
	return m_dirtyRows.size();
}

bool IzSQLUtilities::SQLModel::cleanData()
{
	if (!isDirty()) {
		qWarning() << "Model is already clean.";
		return false;
	}
	for (int i = 0; i < m_modelData.count(); ++i) {
		if (rowIsDirty(i)) {
			m_modelData.at(i)->cleanContainer();
			updateRowState(m_modelData.at(i));
			emit dataChanged(index(i, 0), index(i, 0));
		}
	}
	return true;
}

bool IzSQLUtilities::SQLModel::addRow(const QVariantMap& fieldValues, const QStringList& controlFields)
{
	if (fieldValues.empty()) {
		qWarning() << "Field values map is empty. Row creation is not possible.";
		return false;
	}
	if (getLoadStatus() != ModelLoadStatus::LOADED && getLoadStatus() != ModelLoadStatus::QUERY_EMPTY) {
		qWarning() << "Model is currently in a state that does not permit element creation.";
		return false;
	}
	if (!controlFields.isEmpty()) {
		for (int i = 0; i < m_modelData.size(); ++i) {
			auto hitIterator{ 0 };
			for (const auto& controlField : qAsConst(controlFields)) {
				if (!m_columnIndexMap.contains(controlField)) {
					qCritical() << "Field:" << controlField << "was not found in container definition. Row creation is not possible.";
					return false;
				}
				QVariant value = fieldValues.value(controlField);
				if (value == getFieldValue(i, controlField)) {
					hitIterator++;
				}
			}
			if (hitIterator == controlFields.size()) {
				if (m_replaceFieldsOnAdd) {
					if (m_fieldsToReplace.isEmpty()) {
						qWarning() << "ReplaceFieldsOnAdd is set to true but no fields to replace were provided. Row will be considered as duplicate.";
						emit duplicate();
						return false;
					}
					for (const auto& field : qAsConst(m_fieldsToReplace)) {
						if (!getSQLColumnNames().contains(field)) {
							qWarning() << "Field:" << field << "not found in rows SQL properties. Row will not be added.";
							return false;
						}
						if (getFieldValue(i, field) != fieldValues[field]) {
							if (!setFieldValue(i, field, fieldValues[field])) {
								qWarning() << "There was a problem adding:" << field << "Operation will be stopped.";
								return false;
							}
							updateRowState(m_modelData.at(i));
						} else {
							qInfo() << "Field:" << field << "has duplicate value. It will be skipped.";
						}
					}
					return true;
				}
				emit duplicate();
				return false;
			}
		}
	}
	QVariantMap fields = fieldValues;
	QMapIterator<QString, QVariant> i(m_defaultDatafieldValues);
	while (i.hasNext()) {
		i.next();
		fields.insert(i.key(), i.value());
	}
	QSharedPointer<SQLDataContainer> newElement(new SQLDataContainer(static_cast<unsigned int>(m_indexColumnMap.size()), fields, m_sqlColumnNames));
	if (newElement->isValid()) {
		// TODO: tu emituje się kopia całego kontenera, hmm
		emit rowAboutToBeAdded(dataSize(), newElement->containerData());
		beginInsertRows(QModelIndex(), dataSize(), dataSize());
		m_modelData.append(newElement);
		endInsertRows();
		updateRowState(newElement);
		qInfo() << "Row added.";
		return true;
	}
	qCritical() << "Created row is invalid. Row creation failed.";
	return false;
}

bool IzSQLUtilities::SQLModel::addRows(const QList<QPair<QVariantMap, QStringList>>& rows)
{
	for (const auto& row : qAsConst(rows)) {
		auto res = addRow(row.first, row.second);
		if (!res) {
			qCritical() << "Created row is invalid. Row creation failed.";
			return false;
		}
	}
	return true;
}

bool IzSQLUtilities::SQLModel::toBeRemoved(int row) const
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	return m_modelData.at(row)->toBeRemoved();
}

void IzSQLUtilities::SQLModel::flipToBeRemoved(int row)
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return;
	}
	if (m_modelData.at(row)->toBeRemoved()) {
		m_modelData.at(row)->setToBeRemoved(false);
	} else {
		m_modelData.at(row)->setToBeRemoved(true);
	}
	updateRowState(m_modelData.at(row));
	emit dataChanged(index(row, 0), index(row, 0), QVector<int>{ m_cachedRoleNames.key("__toBeRemoved__") });
}

bool IzSQLUtilities::SQLModel::isAdded(int row) const
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return false;
	}
	return m_modelData.at(row)->isAdded();
}

void IzSQLUtilities::SQLModel::flipIsAdded(int row)
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return;
	}
	if (m_modelData.at(row)->isAdded()) {
		m_modelData.at(row)->setIsAdded(false);
	} else {
		m_modelData.at(row)->setIsAdded(true);
	}
	updateRowState(m_modelData.at(row));
	emit dataChanged(index(row, 0), index(row, 0), QVector<int>{ m_cachedRoleNames.key("__isAdded__") });
}

bool IzSQLUtilities::SQLModel::isInLoadProcess()
{
	return m_isLoadingData;
}

QStringList IzSQLUtilities::SQLModel::getSQLColumnNames() const
{
	return m_sqlColumnNames;
}

QHash<QString, int> IzSQLUtilities::SQLModel::getColumnIndexMap() const
{
	return m_columnIndexMap;
}

int IzSQLUtilities::SQLModel::findFirstRowIndex(const QString& columnName, const QVariant& fieldValue) const
{
	for (auto const& element : m_modelData) {
		if (element->fieldValue(getColumnIndex(columnName)) == fieldValue) {
			return m_modelData.indexOf(element);
		}
	}
	return -1;
}

void IzSQLUtilities::SQLModel::debug_rowData(int row) const
{
	if (!hasIndex(row, 0)) {
		qCritical() << "Invalid index for row:" << row << "requested.";
		return;
	}
	qDebug() << "SQLModel [debug_elementData]" << getRowData(row);
}

void IzSQLUtilities::SQLModel::debug_modelData() const
{
	for (int i = 0; i < dataSize(); ++i) {
		qDebug() << "SQLModel [debug_modelData]" << getRowData(i);
	}
}
