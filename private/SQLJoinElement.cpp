#include "SQLJoinElement.h"

IzSQLUtilities::SQLJoinElement::SQLJoinElement(const QString& tableNanme,
											   const QString& leftTable,
											   const QString& rightTable,
											   const QString& leftConstrain,
											   const QString& rightConstrain,
											   const QString& joinType,
											   const QList<QStringList>& andSubelements)
	: m_tableName(tableNanme)
	, m_leftTable(leftTable)
	, m_rightTable(rightTable)
	, m_leftConstrain(leftConstrain)
	, m_rightConstrain(rightConstrain)
	, m_joinType(joinType)
	, m_andSubelements(andSubelements)
{
}

const QString IzSQLUtilities::SQLJoinElement::stringValue() const
{
	QString joinTemplate = QStringLiteral("_joinType JOIN _rightTableName _rightTableAlias ON _leftTableAlias._leftTableConstrain = _rightTableAlias._rightTableConstrain");

	/* table names */
	joinTemplate.replace(QLatin1String("_rightTableName"), m_tableName);

	/* table aliases */
	joinTemplate.replace(QLatin1String("_rightTableAlias"), m_rightTable);
	joinTemplate.replace(QLatin1String("_leftTableAlias"), m_leftTable);

	/* constrains */
	joinTemplate.replace(QLatin1String("_rightTableConstrain"), m_rightConstrain);
	joinTemplate.replace(QLatin1String("_leftTableConstrain"), m_leftConstrain);

	/* join types */
	if (m_joinType == QLatin1String("left")) {
		joinTemplate.replace(QLatin1String("_joinType"), QLatin1String("LEFT"));
	}

	if (m_joinType == QLatin1String("right")) {
		joinTemplate.replace(QLatin1String("_joinType"), QLatin1String("RIGHT"));
	}

	if (m_joinType == QLatin1String("inner")) {
		joinTemplate.replace(QLatin1String("_joinType"), QLatin1String("INNER"));
	}

	/* ands */
	if (!m_andSubelements.isEmpty()) {
		for (int i = 0; i < m_andSubelements.size(); i++) {
			joinTemplate.append(QStringLiteral(" AND ") + m_andSubelements.at(i).at(0) + QStringLiteral(".") + m_andSubelements.at(i).at(1) + QStringLiteral(" = ") + m_andSubelements.at(i).at(2));
		}
	}

	return joinTemplate;
}

const QString IzSQLUtilities::SQLJoinElement::elementType() const
{
	return QStringLiteral("join");
}
