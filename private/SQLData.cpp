#include "SQLData.h"

#include <QDebug>

#include "IzSQLUtilities/SQLDataContainer.h"

IzSQLUtilities::SQLData::SQLData()
{
	qRegisterMetaType<QSharedPointer<IzSQLUtilities::SQLData>>();
}

QStringList IzSQLUtilities::SQLData::getSQLColumnNames() const
{
	return m_sqlColumnNames;
}

void IzSQLUtilities::SQLData::setSQLColumnNames(const QStringList& columnNames)
{
	m_sqlColumnNames = columnNames;
}

QVector<QSharedPointer<IzSQLUtilities::SQLDataContainer>> IzSQLUtilities::SQLData::sqlData() const
{
	return m_sqlData;
}

void IzSQLUtilities::SQLData::addRow(QSharedPointer<SQLDataContainer> row)
{
	m_sqlData.append(row);
}

QSqlError IzSQLUtilities::SQLData::sqlError() const
{
	return m_sqlError;
}

void IzSQLUtilities::SQLData::setSqlError(const QSqlError& sqlError)
{
	m_sqlError = sqlError;
}

QHash<QString, int> IzSQLUtilities::SQLData::getColumnIndexMap() const
{
	return m_columnIndexMap;
}

void IzSQLUtilities::SQLData::setColumnIndexMap(const QHash<QString, int>& columnIndexMap)
{
	m_columnIndexMap = columnIndexMap;
}

QHash<int, QString> IzSQLUtilities::SQLData::getIndexColumnMap() const
{
	return m_indexColumnMap;
}

void IzSQLUtilities::SQLData::setIndexColumnMap(const QHash<int, QString>& indexColumnMap)
{
	m_indexColumnMap = indexColumnMap;
}

QVector<int> IzSQLUtilities::SQLData::getRemovedElements() const
{
	return m_removedElements;
}

void IzSQLUtilities::SQLData::setRemovedElements(const QVector<int>& removedElements)
{
	m_removedElements = removedElements;
}

QPair<QString, QMap<int, QVariant>> IzSQLUtilities::SQLData::getRefreshedElements() const
{
	return m_refreshedElements;
}

void IzSQLUtilities::SQLData::setRefreshedElements(const QPair<QString, QMap<int, QVariant>>& indexValueMap)
{
	m_refreshedElements = indexValueMap;
}

bool IzSQLUtilities::SQLData::getPartialRefresh() const
{
	return m_partialRefresh;
}

void IzSQLUtilities::SQLData::setPartialRefresh(bool partialRefresh)
{
	m_partialRefresh = partialRefresh;
}

IzSQLUtilities::ModelLoadStatus IzSQLUtilities::SQLData::loadStatus() const
{
	return m_loadStatus;
}

void IzSQLUtilities::SQLData::setLoadStatus(const ModelLoadStatus& loadStatus)
{
	m_loadStatus = loadStatus;
}

void IzSQLUtilities::SQLData::clearData()
{
	m_sqlData.clear();
	m_columnIndexMap.clear();
	m_columnIndexMap.clear();
}
