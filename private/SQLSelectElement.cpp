#include "SQLSelectElement.h"

IzSQLUtilities::SQLSelectElement::SQLSelectElement(const QString& selectType,
												   const QString& tableAlias,
												   const QString& columnAlias,
												   const QString& columnName,
												   const QString& coalesce)
	: m_selectType(selectType)
	, m_tableAlias(tableAlias)
	, m_columnAlias(columnAlias)
	, m_columnName(columnName)
	, m_coalesce(coalesce)
{
}

const QString IzSQLUtilities::SQLSelectElement::stringValue() const
{
	/* COALESCE */
	if (!m_coalesce.isNull()) {
		return QStringLiteral("ISNULL(") + m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(", '") + m_coalesce + QStringLiteral("') AS ") + m_columnAlias;
	}

	/* NO ADDITIONAL PARAMETERS */
	return m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(" AS ") + m_columnAlias;
}

const QString IzSQLUtilities::SQLSelectElement::elementType() const
{
	return QStringLiteral("select");
}

QString IzSQLUtilities::SQLSelectElement::columnAlias() const
{
	return m_columnAlias;
}
