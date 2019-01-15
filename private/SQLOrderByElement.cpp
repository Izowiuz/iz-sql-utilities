#include "SQLOrderByElement.h"

IzSQLUtilities::SQLOrderByElement::SQLOrderByElement(const QString& tableAlias,
													 const QString& tableColumn,
													 const QString& orderType)
	: m_tableAlias(tableAlias)
	, m_tableColumn(tableColumn)
	, m_orderType(orderType)
{
}

const QString IzSQLUtilities::SQLOrderByElement::stringValue() const
{
	if (!m_orderType.isNull()) {
		return m_tableAlias + QStringLiteral(".") + m_tableColumn + QStringLiteral(" ") + m_orderType;
	} else {
		return m_tableAlias + QStringLiteral(".") + m_tableColumn;
	}
}

const QString IzSQLUtilities::SQLOrderByElement::elementType() const
{
	return QStringLiteral("order by");
}

QString IzSQLUtilities::SQLOrderByElement::tableAlias() const
{
	return m_tableAlias;
}

QString IzSQLUtilities::SQLOrderByElement::tableColumn() const
{
	return m_tableColumn;
}
