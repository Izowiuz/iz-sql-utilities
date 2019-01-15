#include "LoadedSQLData.h"

#include <QDebug>

QHash<int, QString> IzSQLUtilities::LoadedSQLData::indexColumnMap() const
{
	return m_indexColumnMap;
}

void IzSQLUtilities::LoadedSQLData::setIndexColumnMap(const QHash<int, QString>& indexColumnMap)
{
	m_indexColumnMap = indexColumnMap;
}

void IzSQLUtilities::LoadedSQLData::addRow(std::unique_ptr<SQLDataContainer> row)
{
	m_sqlData.push_back(std::move(row));
}

QHash<QString, int> IzSQLUtilities::LoadedSQLData::columnIndexMap() const
{
	return m_columnIndexMap;
}

void IzSQLUtilities::LoadedSQLData::setColumnIndexMap(const QHash<QString, int>& columnIndexMap)
{
	m_columnIndexMap = columnIndexMap;
}

std::vector<std::unique_ptr<IzSQLUtilities::SQLDataContainer>>& IzSQLUtilities::LoadedSQLData::sqlData()
{
	return m_sqlData;
}
