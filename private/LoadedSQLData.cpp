﻿#include "LoadedSQLData.h"

QMap<int, QString> IzSQLUtilities::LoadedSQLData::indexColumnMap() const
{
    return m_indexColumnMap;
}

void IzSQLUtilities::LoadedSQLData::setIndexColumnMap(const QMap<int, QString>& indexColumnMap)
{
    m_indexColumnMap = indexColumnMap;
}

void IzSQLUtilities::LoadedSQLData::addRow(std::unique_ptr<SQLRow> row)
{
    m_sqlData.push_back(std::move(row));
}

std::vector<QMetaType>& IzSQLUtilities::LoadedSQLData::sqlDataTypes()
{
    return m_sqlDataTypes;
}

void IzSQLUtilities::LoadedSQLData::setSqlDataTypes(const std::vector<QMetaType>& sqlDataTypes)
{
    m_sqlDataTypes = sqlDataTypes;
}

QHash<QString, int> IzSQLUtilities::LoadedSQLData::columnIndexMap() const
{
    return m_columnIndexMap;
}

void IzSQLUtilities::LoadedSQLData::setColumnIndexMap(const QHash<QString, int>& columnIndexMap)
{
    m_columnIndexMap = columnIndexMap;
}

std::vector<std::unique_ptr<IzSQLUtilities::SQLRow>>& IzSQLUtilities::LoadedSQLData::sqlData()
{
    return m_sqlData;
}
