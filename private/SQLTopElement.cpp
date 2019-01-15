#include "SQLTopElement.h"

IzSQLUtilities::SQLTopElement::SQLTopElement(const QString& type,
											 const QString& value)
	: m_type(type)
	, m_value(value)
{
}

const QString IzSQLUtilities::SQLTopElement::stringValue() const
{
	if (m_type == QLatin1String("integer")) {
		return QStringLiteral("TOP (") + m_value + QStringLiteral(")");
	} else {
		return QStringLiteral("TOP (") + m_value + QStringLiteral(") PERCENT");
	}
}

const QString IzSQLUtilities::SQLTopElement::elementType() const
{
	return QStringLiteral("top");
}

QString IzSQLUtilities::SQLTopElement::value() const
{
	return m_value;
}
