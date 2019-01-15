#include "SQLTable.h"

#include <QDebug>

IzSQLUtilities::SQLTable::SQLTable(const QString& tableName,
								   const QString& tableAlias)
	: m_tableName(tableName)
	, m_tableAlias(tableAlias)
{
}

QString IzSQLUtilities::SQLTable::tableName() const
{
	return m_tableName;
}

void IzSQLUtilities::SQLTable::setTableName(const QString& tableName)
{
	m_tableName = tableName;
}

QString IzSQLUtilities::SQLTable::tableAlias() const
{
	return m_tableAlias;
}

void IzSQLUtilities::SQLTable::setTableAlias(const QString& tableAlias)
{
	m_tableAlias = tableAlias;
}

bool IzSQLUtilities::SQLTable::addTableColumn(const QString& columnAlias, const QString& columnName)
{
	QHashIterator<QString, QString> i(m_tableColumns);
	while (i.hasNext()) {
		i.next();
		if (i.key() == columnAlias && i.value() == columnName) {
			qCritical() << "Column:" << columnName << "with alias:" << columnAlias << "already exists in table:" + tableName();
			return false;
		}
	}
	m_tableColumns.insert(columnAlias, columnName);
	return true;
}

QHash<QString, QString> IzSQLUtilities::SQLTable::tableColumns() const
{
	return m_tableColumns;
}

bool IzSQLUtilities::SQLTable::hasColumnAlias(const QString& columnAlias) const
{
	if (m_tableColumns.contains(columnAlias)) {
		return true;
	}
	qCritical() << "No column with alias:" << columnAlias << "found for table:" + tableAlias();
	return false;
}

bool IzSQLUtilities::SQLTable::hasColumn(const QString& columnName) const
{
	QHashIterator<QString, QString> i(m_tableColumns);
	while (i.hasNext()) {
		i.next();
		if (i.value() == columnName) {
			return true;
		}
	}
	qCritical() << "No column:" << columnName << "found for table:" + tableAlias();
	return false;
}

const QString IzSQLUtilities::SQLTable::getColumnName(const QString& columnAlias) const
{
	if (m_tableColumns.contains(columnAlias)) {
		return m_tableColumns.value(columnAlias);
	}
	qCritical() << "No column with column alias:" << columnAlias << "found.";
	return QString();
}
