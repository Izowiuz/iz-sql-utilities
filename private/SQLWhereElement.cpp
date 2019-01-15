#include "SQLWhereElement.h"

IzSQLUtilities::SQLWhereElement::SQLWhereElement(const QString& tableAlias,
												 const QString& columnName,
												 const QString& whereValue)
	: m_tableAlias(tableAlias)
	, m_columnName(columnName)
	, m_whereValue(whereValue)
{
}

const QString IzSQLUtilities::SQLWhereElement::stringValue() const
{
	QString tmpValue = m_whereValue;

	tmpValue.remove(QStringLiteral("DELETE"), Qt::CaseInsensitive);
	tmpValue.remove(QStringLiteral("UPDATE"), Qt::CaseInsensitive);
	tmpValue.remove(QStringLiteral("SELECT"), Qt::CaseInsensitive);
	tmpValue.remove(QStringLiteral("FROM"), Qt::CaseInsensitive);
	tmpValue.remove(QStringLiteral("LIKE"), Qt::CaseInsensitive);
	tmpValue.remove(QStringLiteral("NOT LIKE"), Qt::CaseInsensitive);
	tmpValue.remove(QStringLiteral("TRUNCATE"), Qt::CaseInsensitive);
	tmpValue.remove(QStringLiteral("DROP"), Qt::CaseInsensitive);

	if (tmpValue.left(1) == QLatin1String("!")) {
		tmpValue.remove(0, 1);
		if (tmpValue.contains(QLatin1String("_")) && !tmpValue.contains(QLatin1String("*"))) {
			tmpValue.append(QLatin1String("%"));
		}
		tmpValue.replace(QLatin1String("*"), QLatin1String("%"));
		return m_tableAlias % "." % m_columnName % " NOT LIKE " % "'" % tmpValue % "'";
	}

	if (tmpValue == QLatin1String("@")) {
		tmpValue.remove(0, 1);
		return m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(" IS NULL OR ") + m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(" = ''");
	}

	if (tmpValue == QLatin1String("!@")) {
		tmpValue.remove(0, 2);
		return m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(" IS NOT NULL OR ") + m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(" != ''");
	}

	if (tmpValue.contains(QLatin1String("_"))) {
		tmpValue.replace(QLatin1String("*"), QLatin1String("%"));
		return m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(" LIKE ") + QStringLiteral("'") + tmpValue + QStringLiteral("%'");
	}

	if (tmpValue.contains(QLatin1String("*"))) {
		tmpValue.replace(QLatin1String("*"), QLatin1String("%"));
		return m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(" LIKE ") + QStringLiteral("'") + tmpValue + QStringLiteral("'");
	}

	return m_tableAlias + QStringLiteral(".") + m_columnName + QStringLiteral(" = ") + QStringLiteral("'") + tmpValue + QStringLiteral("'");
}

const QString IzSQLUtilities::SQLWhereElement::elementType() const
{
	return QStringLiteral("where");
}

QString IzSQLUtilities::SQLWhereElement::tableAlias() const
{
	return m_tableAlias;
}

QString IzSQLUtilities::SQLWhereElement::columnName() const
{
	return m_columnName;
}

QString IzSQLUtilities::SQLWhereElement::whereValue() const
{
	return m_whereValue;
}

bool IzSQLUtilities::SQLWhereElement::isDynamic() const
{
	return m_isDynamic;
}

void IzSQLUtilities::SQLWhereElement::setIsDynamic(bool isDynamic)
{
	m_isDynamic = isDynamic;
}

int IzSQLUtilities::SQLWhereElement::getFilterID() const
{
	return m_filterID;
}

void IzSQLUtilities::SQLWhereElement::setFilterID(int value)
{
	m_filterID = value;
}

void IzSQLUtilities::SQLWhereElement::setWhereValue(const QString& whereValue)
{
	m_whereValue = whereValue;
}
