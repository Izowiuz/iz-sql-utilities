#include "SQLElement.h"

#include "SQLTable.h"

QSharedPointer<IzSQLUtilities::SQLTable> IzSQLUtilities::SQLElement::parentTable() const
{
	return m_parentTable;
}

void IzSQLUtilities::SQLElement::setParentTable(QSharedPointer<SQLTable> parentTable)
{
	m_parentTable = parentTable;
}
