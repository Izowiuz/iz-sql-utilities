#include "IzSQLUtilities/SQLQueryBuilder.h"

#include <QDebug>

#include "SQLElement.h"
#include "SQLJoinElement.h"
#include "SQLOrderByElement.h"
#include "SQLSelectElement.h"
#include "SQLTable.h"
#include "SQLTopElement.h"
#include "SQLWhereElement.h"

IzSQLUtilities::SQLQueryBuilder::SQLQueryBuilder(QObject* parent)
	: QObject(parent)
	, m_viewIsInitialized(false)
	, m_queryCorrupted(false)
	, m_errorFound(false)
	, m_topApplied(false)
{
	qRegisterMetaType<IzSQLUtilities::SQLQueryBuilder*>();
}

void IzSQLUtilities::SQLQueryBuilder::setView(const QString& query, const QString& orderBy)
{
	m_view              = query;
	m_viewOrderBy       = orderBy;
	m_viewIsInitialized = true;
}

void IzSQLUtilities::SQLQueryBuilder::clearView()
{
	m_view.clear();
	m_viewFilters.clear();
	m_viewIsInitialized = false;
}

const QString IzSQLUtilities::SQLQueryBuilder::getView()
{
	QString query = m_view;

	if (m_viewFilters.count() > 0) {
		query.append(" WHERE ");

		QHashIterator<int, QString> i(m_viewFilters);
		while (i.hasNext()) {
			i.next();
			query.append(i.value());
			query.append("AND ");
		}
		query.remove(query.length() - 4, 3);
	}

	if (m_viewOrderBy.isEmpty()) {
		return query;
	}
	return query % " ORDER BY " % m_viewOrderBy;
}

void IzSQLUtilities::SQLQueryBuilder::addViewFilter(int filterID, const QVariantMap& filterDefinition)
{
	// *	-> like
	// @	-> empty
	// !*	-> not like
	// !@	-> not empty

	QString tmpFilter;
	QString filterValue = pVal(filterDefinition, "value");

	filterValue.remove("DELETE", Qt::CaseInsensitive);
	filterValue.remove("UPDATE", Qt::CaseInsensitive);
	filterValue.remove("SELECT", Qt::CaseInsensitive);
	filterValue.remove("FROM", Qt::CaseInsensitive);
	filterValue.remove("LIKE", Qt::CaseInsensitive);
	filterValue.remove("NOT LIKE", Qt::CaseInsensitive);
	filterValue.remove("TRUNCATE", Qt::CaseInsensitive);
	filterValue.remove("DROP", Qt::CaseInsensitive);
	filterValue.remove("CREATE", Qt::CaseInsensitive);

	/* jeżeli w filtr o takim ID został już zdefiniowany usuwamy go */
	if (m_viewFilters.contains(filterID)) {
		m_viewFilters.remove(filterID);
	}

	/* przeparsowanie wartości filtru */
	if (pVal(filterDefinition, "contextSearch") == QString("true")) {
		if (filterValue.left(1) == "*") {
			filterValue = filterValue.remove(0, 1);
			tmpFilter.append(pVal(filterDefinition, "column") % " LIKE '%" % filterValue % "%' ");
		} else if (filterValue.left(1) == "@") {
			tmpFilter.append("(" % pVal(filterDefinition, "column") % " IS NULL OR " % pVal(filterDefinition, "column") % " = '' ) ");
		} else if (filterValue.left(2) == "!*") {
			filterValue = filterValue.remove(0, 2);
			tmpFilter.append(pVal(filterDefinition, "column") % " NOT LIKE '%" % filterValue % "%' ");
		} else if (filterValue.left(2) == "!@") {
			tmpFilter.append("(" % pVal(filterDefinition, "column") % " IS NOT NULL OR " % pVal(filterDefinition, "column") % " != '' ) ");
		} else {
			tmpFilter.append(pVal(filterDefinition, "column") % " = '" % filterValue % "' ");
		}
	} else {
		tmpFilter.append(pVal(filterDefinition, "column") % " = '" % filterValue % "' ");
	}

	m_viewFilters.insert(filterID, tmpFilter);
}

void IzSQLUtilities::SQLQueryBuilder::removeViewFilter(int filterID)
{
	m_viewFilters.remove(filterID);
}

bool IzSQLUtilities::SQLQueryBuilder::getViewIsInitialized() const
{
	return m_viewIsInitialized;
}

const QString IzSQLUtilities::SQLQueryBuilder::pVal(const QVariantMap& parameters, const QString& parameter) const
{
	return parameters.value(parameter).toString();
}

bool IzSQLUtilities::SQLQueryBuilder::checkParameter(const QVariantMap& parameters, const QString& parameter) const
{
	if (parameters.empty()) {
		qCritical() << "Empty parameter container.";
		return false;
	}
	if (parameters.contains(parameter)) {
		if (!parameters.value(parameter).isNull()) {
			return true;
		}
		qCritical() << "Value for parameter:" << parameter << "is empty.";
		return false;
	}
	qCritical() << "Parameter:" << parameter << "not found.";
	return false;
}

void IzSQLUtilities::SQLQueryBuilder::addTable(const QVariantMap& tableDefinition, const QVariantList& tableColumns)
{
	/* sanity check */
	if (!sanityCheck("addTable")) {
		return;
	}

	if (checkParameter(tableDefinition, "tableName") && checkParameter(tableDefinition, "tableAlias")) {

		QString tableName  = pVal(tableDefinition, "tableName");
		QString tableAlias = pVal(tableDefinition, "tableAlias");

		/* if there is already added table with this alias invalidate query */
		if (tableExists(tableAlias, true)) {
			qCritical() << "Table with alias:" << tableAlias << "is already defined. Aborting.";
			invalidateQuery();
			return;
		}
		if (tableColumns.empty()) {
			qCritical() << "Error adding table:" << tableName << "with alias:" << tableAlias << "Empty parameter list.";
			invalidateQuery();
			return;
		}

		QSharedPointer<SQLTable> sqlTable = QSharedPointer<SQLTable>(new SQLTable(tableName, tableAlias));

		for (int i = 0; i < tableColumns.size(); i++) {
			QVariantMap tmpColumn = tableColumns.at(i).toMap();

			/* check for necessary column parameters */
			if (!checkParameter(tmpColumn, "columnName") || !checkParameter(tmpColumn, "columnAlias")) {
				qCritical() << "One of the column parameters is invalid. Aborting.";
				invalidateQuery();
				return;
			}

			QString columnName  = pVal(tmpColumn, "columnName");
			QString columnAlias = pVal(tmpColumn, "columnAlias");

			if (!sqlTable->addTableColumn(columnAlias, columnName)) {
				qCritical() << "Error adding table column.";
				invalidateQuery();
				return;
			}
		}

		m_sqlTables.append(sqlTable);
		return;
	}
	qWarning() << "Error adding table column.";
	invalidateQuery();
}

void IzSQLUtilities::SQLQueryBuilder::removeTable(const QVariantMap& tableDefinition)
{
	/* sanity check */
	if (!sanityCheck("removeTable")) {
		return;
	}

	if (m_sqlTables.isEmpty()) {
		qCritical() << "SQL tables list is empty.";
		return;
	}

	if (checkParameter(tableDefinition, "tableAlias")) {
		QString tableAlias                   = pVal(tableDefinition, "tableAlias");
		QSharedPointer<SQLTable> parentTable = getTableFromAlias(tableAlias);

		if (parentTable != nullptr) {
			m_sqlTables.removeAt(m_sqlTables.indexOf(parentTable));

			/* remove child elements */
			QMutableListIterator<QSharedPointer<SQLElement>> i(m_queryElements);
			while (i.hasNext()) {
				i.next();
				if (i.value()->parentTable() == parentTable) {
					i.remove();
				}
			}
			/* remove related joins */
			return;
		}
		qWarning() << "Table with alias:" << tableAlias << "not found.";
	}
	qWarning() << "Error removing table.";
}

void IzSQLUtilities::SQLQueryBuilder::join(const QVariantMap& tables, const QVariantMap& joinParameters)
{
	/* sanity check */
	if (!sanityCheck("join")) {
		return;
	}

	/* temporary variables */
	QString tableName;
	QString leftTable;
	QString rightTable;
	QString leftConstrain;
	QString rightConstrain;
	QString joinType;
	QList<QStringList> andConstrains;
	QList<QStringList> whereConstrains;
	QSharedPointer<SQLTable> parentTable;

	/* check for the necessary table parameters */
	if (!checkParameter(tables, "leftTable") || !checkParameter(tables, "rightTable")) {
		qCritical() << "One of the table parameters is invalid. Aborting.";
		invalidateQuery();
		return;
	}

	/* check for necessary join parameters */
	if (
		!checkParameter(joinParameters, "leftConstrain") || !checkParameter(joinParameters, "rightConstrain") || !checkParameter(joinParameters, "type")) {
		qCritical() << "One of the join parameters is invalid. Aborting.";
		invalidateQuery();
		return;
	}

	/* check for valid parameters values */
	if (!checkParameterValue(pVal(joinParameters, "type"), QStringList{ "left", "right", "inner" })) {
		qCritical() << "Incorrect parameter value found. Aborting.";
		invalidateQuery();
		return;
	}

	leftTable      = pVal(tables, "leftTable");
	rightTable     = pVal(tables, "rightTable");
	leftConstrain  = pVal(joinParameters, "leftConstrain");
	rightConstrain = pVal(joinParameters, "rightConstrain");
	joinType       = pVal(joinParameters, "type");
	parentTable    = getTableFromAlias(rightTable);

	/* check for the tables */
	if (!tableExists(leftTable) || !tableExists(rightTable)) {
		qCritical() << "One of the requested tables does not exist. Aborting.";
		invalidateQuery();
		return;
	}

	/* check for the column aliases */
	if (!getTableFromAlias(leftTable)->hasColumn(leftConstrain) || !getTableFromAlias(rightTable)->hasColumn(rightConstrain)) {
		qCritical() << "One of the requested columns does not exist. Aborting.";
		invalidateQuery();
		return;
	}

	/* if 'and' parameter exists we have to parse its contents */
	if (!joinParameters.value("and").isNull()) {

		QVariantList tmpElements = joinParameters.value("and").toList();

		foreach (QVariant tmpElement, tmpElements) {
			QVariantMap tmpMap = tmpElement.toMap();

			/* check for proper 'and' parameters */
			if (
				!checkParameter(tmpMap, "tableAlias") || !checkParameter(tmpMap, "columnName") || !checkParameter(tmpMap, "constrainValue")) {
				qCritical() << "'and' defined but one of the parameters is invalid. Aborting.";
				invalidateQuery();
				return;
			}

			QString tableAlias = pVal(tmpMap, "tableAlias");
			QString columnName = pVal(tmpMap, "columnName");
			QString andValue   = pVal(tmpMap, "constrainValue");

			/* check for proper table defined in 'and' */
			if (!tableExists(tableAlias)) {
				qCritical() << "Table defined in 'and' parameter does not exist. Aborting.";
				invalidateQuery();
				return;
			}

			/* check for proper column in table */
			if (!getTableFromAlias(tableAlias)->hasColumn(columnName)) {
				qCritical() << "Column defined in 'and' parameter does not exist in table with alias " % tableAlias % " Aborting.";
				invalidateQuery();
				return;
			}
			andConstrains.append(QStringList{ tableAlias, columnName, andValue });
		}
	}

	/* if 'where' parameter exists we have to parse its contents */
	if (!joinParameters.value("and").isNull()) {

		QVariantList tmpElements = joinParameters.value("where").toList();

		foreach (QVariant tmpElement, tmpElements) {
			QVariantMap tmpMap = tmpElement.toMap();

			/* check for proper 'where' parameters */
			if (
				!checkParameter(tmpMap, "tableAlias") || !checkParameter(tmpMap, "columnName") || !checkParameter(tmpMap, "whereValue")) {
				qCritical() << "'where' defined but one of the parameters is invalid. Aborting.";
				invalidateQuery();
				return;
			}

			QString tableAlias = pVal(tmpMap, "tableAlias");
			QString columnName = pVal(tmpMap, "columnName");
			QString whereValue = pVal(tmpMap, "whereValue");

			/* check for proper table defined in 'where' */
			if (!tableExists(tableAlias)) {
				qCritical() << "Table defined in 'where' parameter does not exist. Aborting.";
				invalidateQuery();
				return;
			}

			/* check for proper column in table */
			if (!getTableFromAlias(tableAlias)->hasColumn(columnName)) {
				qCritical() << "Column defined in 'where' parameter does not exist in table with alias:" << tableAlias << "Aborting.";
				invalidateQuery();
				return;
			}

			whereConstrains.append(QStringList{ tableAlias, columnName, whereValue });
		}
	}

	/* we've got all the elements, time to create necessary objects and add them to the internal data */
	tableName = getTableFromAlias(rightTable)->tableName();
	QSharedPointer<SQLElement> joinElement(new SQLJoinElement(tableName, leftTable, rightTable, leftConstrain, rightConstrain, joinType, andConstrains));
	joinElement->setParentTable(parentTable);
	m_queryElements.append(joinElement);

	for (int i = 0; i < whereConstrains.size(); i++) {

		QString tableAlias = whereConstrains.at(i).at(0);
		QString columnName = whereConstrains.at(i).at(1);
		QString whereValue = whereConstrains.at(i).at(2);

		/* check for already defined where element */
		/* duplicate where = tableAlias + columnName + whereValue already defined in where element */
		bool isDuplicate = false;
		for (int i = 0; i < m_queryElements.size(); i++) {
			if (m_queryElements.at(i)->elementType() == "where" && m_queryElements.at(i).staticCast<SQLWhereElement>()->tableAlias() == tableAlias && m_queryElements.at(i).staticCast<SQLWhereElement>()->columnName() == columnName && m_queryElements.at(i).staticCast<SQLWhereElement>()->whereValue() == whereValue) {
				qWarning() << "Where for table:" << tableAlias << "and column" << columnName << "with value" << whereValue << "already defined. Skipping.";
				isDuplicate = true;
			}
		}

		if (isDuplicate == false) {
			QSharedPointer<SQLElement> whereElement(new SQLWhereElement(tableAlias, columnName, whereValue));
			whereElement->setParentTable(parentTable);
			m_queryElements.append(whereElement);
		}
	}
}

void IzSQLUtilities::SQLQueryBuilder::top(const QVariantMap& top)
{
	/* sanity check */
	if (!sanityCheck("top")) {
		return;
	}

	/* temporary variables we will use */
	QString type;
	QString value;

	/* check for necessary top parameters */
	if (!checkParameter(top, "type") || !checkParameter(top, "value")) {
		qCritical() << "One of the top parameters is invalid. Aborting.";
		invalidateQuery();
		return;
	}

	/* check for its proper declaration top types */
	if (!type.isNull()) {
		if (!checkParameterValue(pVal(top, "type"), QStringList{ "integer", "percentage" })) {
			qCritical() << "Incorrect parameter value in top found. Aborting.";
			invalidateQuery();
			return;
		}
	}

	type  = pVal(top, "type");
	value = pVal(top, "value");

	/* remove old top element */
	removeTop();

	QSharedPointer<SQLElement> _topElement(new SQLTopElement(type, value));
	m_queryElements.append(_topElement);
	m_topApplied = true;
}

void IzSQLUtilities::SQLQueryBuilder::removeTop()
{
	QMutableListIterator<QSharedPointer<SQLElement>> i(m_queryElements);
	while (i.hasNext()) {
		i.next();
		if (i.value()->elementType() == "top") {
			i.remove();
			m_topApplied = false;
		}
	}
}

void IzSQLUtilities::SQLQueryBuilder::orderBy(const QVariantList& order)
{
	/* sanity check */
	if (!sanityCheck("orderBy")) {
		return;
	}

	for (auto const& orderElement : qAsConst(order)) {
		QVariantMap tmpElement = orderElement.toMap();

		/* check for necessary order parameters */
		if (!checkParameter(tmpElement, "tableAlias") || !checkParameter(tmpElement, "tableColumn")) {
			qCritical() << "One of the order by parameters is invalid. Aborting.";
			invalidateQuery();
			return;
		}

		QString tableAlias                   = pVal(tmpElement, "tableAlias");
		QString tableColumn                  = pVal(tmpElement, "tableColumn");
		QString orderType                    = pVal(tmpElement, "order");
		QSharedPointer<SQLTable> parentTable = getTableFromAlias(tableAlias);

		/* check for the table */
		if (!tableExists(tableAlias)) {
			qCritical() << "One of the requested tables does not exist. Aborting.";
			invalidateQuery();
			return;
		}

		/* check for the column */
		if (!getTableFromAlias(tableAlias)->hasColumn(tableColumn)) {
			qCritical() << "One of the requested columns does not exist. Aborting.";
			invalidateQuery();
			return;
		}

		/* if 'order' parameter is defined check for its proper definition */
		if (!pVal(tmpElement, "order").isNull()) {
			if (!checkParameterValue(pVal(tmpElement, "order"), QStringList{ "ASC", "DESC" })) {
				qCritical() << "Incorrect parameter value found. Aborting.";
				invalidateQuery();
				return;
			}
		}

		/* check for already defined order by element */
		/* duplicate order by = tableAlias + tableColumn already defined in order by element */
		bool isDuplicate = false;
		for (int i = 0; i < m_queryElements.size(); i++) {
			if (m_queryElements.at(i)->elementType() == "order by" && m_queryElements.at(i).staticCast<SQLOrderByElement>()->tableAlias() == tableAlias && m_queryElements.at(i).staticCast<SQLOrderByElement>()->tableColumn() == tableColumn) {
				qWarning() << "Order by for table:" << tableAlias << "and column" << tableColumn << "already defined. Skipping.";
				isDuplicate = true;
			}
		}

		if (isDuplicate == false) {
			QSharedPointer<SQLElement> orderByElement(new SQLOrderByElement(tableAlias, tableColumn, orderType));
			orderByElement->setParentTable(parentTable);
			m_queryElements.append(orderByElement);
		}
	}
}

void IzSQLUtilities::SQLQueryBuilder::addFilter(const QVariantMap& filterDefinition)
{
	/* sanity check */
	if (!sanityCheck("addFilter")) {
		return;
	}

	/* temporary variables */
	QString filterID;
	QString tableAlias;
	QString columnAlias;
	QString filterValue;
	QString columnName;
	QSharedPointer<SQLTable> parentTable;

	/* check for the necessary filter parameters */
	if (!checkParameter(filterDefinition, "filterID") || !checkParameter(filterDefinition, "tableAlias") || !checkParameter(filterDefinition, "columnAlias") || !checkParameter(filterDefinition, "filterValue")) {
		qCritical() << "One of the filter parameters is invalid. Aborting.";
		invalidateQuery();
		return;
	}

	filterID    = pVal(filterDefinition, "filterID");
	tableAlias  = pVal(filterDefinition, "tableAlias");
	columnAlias = pVal(filterDefinition, "columnAlias");
	filterValue = pVal(filterDefinition, "filterValue");

	/* check for the table */
	if (!tableExists(tableAlias)) {
		qCritical() << "One of the requested tables does not exist. Aborting.";
		invalidateQuery();
		return;
	}

	/* check for the column alias */
	if (!getTableFromAlias(tableAlias)->hasColumnAlias(columnAlias)) {
		qCritical() << "One of the requested columns does not exist. Aborting.";
		invalidateQuery();
		return;
	}

	/* generate list of filter values and trim whitespaces */
	QStringList values;
	foreach (QString value, filterValue.split("&")) {
		values.append(value.trimmed());
	}

	columnName  = getTableFromAlias(tableAlias)->getColumnName(columnAlias);
	parentTable = getTableFromAlias(tableAlias);

	QMutableListIterator<QSharedPointer<SQLElement>> i(m_queryElements);
	while (i.hasNext()) {
		i.next();
		if (i.value()->elementType() == "where" && i.value().staticCast<SQLWhereElement>()->getFilterID() == filterID.toInt() && i.value().staticCast<SQLWhereElement>()->isDynamic() == true) {

			/* remove inactive filters */
			if (!values.contains(i.value().staticCast<SQLWhereElement>()->whereValue())) {
				qInfo() << "Removed inactive filter:" << i.value().staticCast<SQLWhereElement>()->getFilterID() << "with value:" << i.value().staticCast<SQLWhereElement>()->whereValue();
				i.remove();
			} else {
				/* leave only new filters */
				values.removeAll(i.value().staticCast<SQLWhereElement>()->whereValue());
				qInfo() << "'were' for table:" << tableAlias << "and column:" << columnName << "with value:" << i.value().staticCast<SQLWhereElement>()->whereValue() << "already defined. Skipping.";
			}
		}
	}

	/* add new filters */
	foreach (QString value, values) {
		QSharedPointer<SQLElement> whereElement(new SQLWhereElement(tableAlias, columnName, value));
		whereElement.staticCast<SQLWhereElement>()->setIsDynamic(true);
		whereElement.staticCast<SQLWhereElement>()->setFilterID(filterID.toInt());
		whereElement->setParentTable(parentTable);
		m_queryElements.append(whereElement);
	}
}

void IzSQLUtilities::SQLQueryBuilder::removeFilter(const QVariantMap& filterDefinition)
{
	/* sanity check */
	if (!sanityCheck("removeFilter")) {
		return;
	}

	//	/* check for the necessary filter parameters */
	if (!checkParameter(filterDefinition, "filterID")) {
		qCritical() << "One of the filter parameters is invalid. Aborting.";
		invalidateQuery();
		return;
	}

	QString filterID = pVal(filterDefinition, "filterID");

	/* remove filter */
	int iterator = 0;
	QMutableListIterator<QSharedPointer<SQLElement>> i(m_queryElements);
	while (i.hasNext()) {
		i.next();
		if (i.value()->elementType() == "where" && i.value().staticCast<SQLWhereElement>()->isDynamic() && i.value().staticCast<SQLWhereElement>()->getFilterID() == filterID.toInt()) {
			i.remove();
		}
		iterator++;
	}
	if (iterator == 0) {
		qWarning() << "No filter with ID:" << filterID << "found.";
	}
}

void IzSQLUtilities::SQLQueryBuilder::changeFilter(const QVariantMap& filterDefinition)
{
	/* sanity check */
	if (!sanityCheck("removeFilter")) {
		return;
	}

	/* temporary variables */
	QString filterID;
	QString filterValue;

	/* check for the necessary filter parameters */
	if (!checkParameter(filterDefinition, "filterID") || !checkParameter(filterDefinition, "filterValue")) {
		qCritical() << "One of the filter parameters is invalid. Aborting";
		invalidateQuery();
		return;
	}

	filterID    = pVal(filterDefinition, "filterID");
	filterValue = pVal(filterDefinition, "filterValue");

	/* change filter value */
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == "where" && m_queryElements.at(i).staticCast<SQLWhereElement>()->isDynamic() && m_queryElements.at(i).staticCast<SQLWhereElement>()->getFilterID() == filterID.toInt()) {
			m_queryElements.at(i).staticCast<SQLWhereElement>()->setWhereValue(filterValue);
			return;
		}
	}
	qWarning() << "No filter with ID:" << filterID << "found.";
}

const QString IzSQLUtilities::SQLQueryBuilder::getQuery()
{
	if (!queryIsValid()) {
		return QString();
	}

	/* temporary variables */
	bool whereApplied   = false;
	bool orderByApplied = false;

	QString tmpQuery = "SELECT ";

	/* TOP ELEMENT */
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == "top") {
			tmpQuery.append(m_queryElements.at(i)->stringValue() % " ");
		}
	}

	/* SELECT ELEMENTS */
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == "select") {
			tmpQuery.append(m_queryElements.at(i)->stringValue() % ", ");
		}
	}

	/* query string fix */
	tmpQuery.remove(tmpQuery.length() - 2, 1);

	/* FROM ELEMENT */
	tmpQuery.append("FROM " % m_sqlTables.first()->tableName() % " AS " % m_sqlTables.first()->tableAlias());

	/* query string fix */
	tmpQuery.append(" ");

	/* JOIN ELEMENTS */
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == "join") {
			tmpQuery.append(m_queryElements.at(i)->stringValue());

			/* query string fix */
			tmpQuery.append(" ");
		}
	}

	/* WHERE ELEMENTS */
	QHash<int, QStringList> filters;

	for (int i = 0; i < m_queryElements.size(); i++) {

		int filterID        = m_queryElements.at(i).staticCast<SQLWhereElement>()->getFilterID();
		QString filterValue = m_queryElements.at(i).staticCast<SQLWhereElement>()->stringValue();

		if (m_queryElements.at(i)->elementType() == "where") {
			if (filters.contains(filterID)) {
				QStringList& tmp = filters[filterID];
				tmp.append(filterValue);
			} else {
				filters.insert(filterID, QStringList{ filterValue });
			}
		}
	}

	QHashIterator<int, QStringList> hash_iterator(filters);
	while (hash_iterator.hasNext()) {
		hash_iterator.next();
		QStringList tmpList = hash_iterator.value();
		if (whereApplied == false) {
			tmpQuery.append("WHERE ");
			if (tmpList.count() > 1) {
				tmpQuery.append("(");
				for (int i = 0; i < tmpList.size(); i++) {
					if (i != tmpList.size() - 1) {
						tmpQuery.append(tmpList.at(i) % " OR ");
					} else {
						tmpQuery.append(tmpList.at(i));
					}
				}
				tmpQuery.append(") ");
			} else {
				tmpQuery.append(" (" % hash_iterator.value().first() % ") ");
			}
			whereApplied = true;
		} else {
			if (tmpList.count() > 1) {
				tmpQuery.append("AND (");
				for (int i = 0; i < tmpList.size(); i++) {
					if (i < tmpList.size() - 1) {
						tmpQuery.append(tmpList.at(i) % " OR ");
					} else {
						tmpQuery.append(tmpList.at(i));
					}
				}
				tmpQuery.append(") ");
			} else {
				tmpQuery.append("AND (" % hash_iterator.value().first() % ") ");
			}
		}
	}

	/* ORDER BY ELEMENTS */
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == "order by") {
			if (orderByApplied == false) {
				tmpQuery.append("ORDER BY ");
				orderByApplied = true;
			}
			tmpQuery.append(m_queryElements.at(i)->stringValue() % ", ");
		}
	}

	if (orderByApplied) {
		/* query string fix */
		tmpQuery.remove(tmpQuery.length() - 2, 2);
	}
	//	IzLogger::Logger::instance()->LOG(IzLogger::LogSeverity::DEBUG, "QUERY: " % tmpQuery);
	return tmpQuery;
}

const QString IzSQLUtilities::SQLQueryBuilder::getCountQuery()
{
	if (!queryIsValid()) {
		return QString();
	}

	/* temporary variables */
	bool whereApplied = false;

	QString tmpQuery = "SELECT ";

	if (m_topApplied == true) {
		for (int i = 0; i < m_queryElements.size(); i++) {
			if (m_queryElements.at(i)->elementType() == "top") {
				tmpQuery.append(m_queryElements.at(i).staticCast<SQLTopElement>()->value());
				return tmpQuery;
			}
		}
	}

	/* COUNT ELEMENT */
	tmpQuery.append("COUNT(*) ");

	/* FROM ELEMENT */
	tmpQuery.append("FROM " % m_sqlTables.first()->tableName() % " AS " % m_sqlTables.first()->tableAlias());

	/* query string fix */
	tmpQuery.append(" ");

	/* JOIN ELEMENTS */
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == "join") {
			tmpQuery.append(m_queryElements.at(i)->stringValue());

			/* query string fix */
			tmpQuery.append(" ");
		}
	}

	/* WHERE ELEMENTS */
	QHash<int, QStringList> filters;

	for (int i = 0; i < m_queryElements.size(); i++) {

		int filterID        = m_queryElements.at(i).staticCast<SQLWhereElement>()->getFilterID();
		QString filterValue = m_queryElements.at(i).staticCast<SQLWhereElement>()->stringValue();

		if (m_queryElements.at(i)->elementType() == "where") {
			if (filters.contains(filterID)) {
				QStringList& tmp = filters[filterID];
				tmp.append(filterValue);
			} else {
				filters.insert(filterID, QStringList{ filterValue });
			}
		}
	}

	QHashIterator<int, QStringList> hash_iterator(filters);
	while (hash_iterator.hasNext()) {
		hash_iterator.next();
		QStringList tmpList = hash_iterator.value();
		if (whereApplied == false) {
			tmpQuery.append("WHERE ");
			if (tmpList.count() > 1) {
				tmpQuery.append("(");
				for (int i = 0; i < tmpList.size(); i++) {
					if (i != tmpList.size() - 1) {
						tmpQuery.append(tmpList.at(i) % " OR ");
					} else {
						tmpQuery.append(tmpList.at(i));
					}
				}
				tmpQuery.append(") ");
			} else {
				tmpQuery.append(" (" % hash_iterator.value().first() % ") ");
			}
			whereApplied = true;
		} else {
			if (tmpList.count() > 1) {
				tmpQuery.append("AND (");
				for (int i = 0; i < tmpList.size(); i++) {
					if (i < tmpList.size() - 1) {
						tmpQuery.append(tmpList.at(i) % " OR ");
					} else {
						tmpQuery.append(tmpList.at(i));
					}
				}
				tmpQuery.append(") ");
			} else {
				tmpQuery.append("AND (" % hash_iterator.value().first() % ") ");
			}
		}
	}

	/* query string fix */
	tmpQuery.remove(tmpQuery.length() - 1, 1);
	//	IzLogger::Logger::instance()->LOG(IzLogger::LogSeverity::DEBUG, "QUERY: " % tmpQuery);
	return tmpQuery;
}

const QString IzSQLUtilities::SQLQueryBuilder::getPartialQuery(const QString& identityColumn, const QString& identityTable, const QVariantList& elementsToRefresh)
{
	if (!queryIsValid()) {
		return QString();
	}

	/* temporary variables */
	bool whereApplied = false;

	QString tmpQuery = "SELECT ";

	/* SELECT ELEMENTS */
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == "select") {
			tmpQuery.append(m_queryElements.at(i)->stringValue() % ", ");
		}
	}

	/* query string fix */
	tmpQuery.remove(tmpQuery.length() - 2, 1);

	/* FROM ELEMENT */
	tmpQuery.append("FROM " % m_sqlTables.first()->tableName() % " AS " % m_sqlTables.first()->tableAlias());

	/* query string fix */
	tmpQuery.append(" ");

	/* JOIN ELEMENTS */
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == "join") {
			tmpQuery.append(m_queryElements.at(i)->stringValue());

			/* query string fix */
			tmpQuery.append(" ");
		}
	}

	/* WHERE ELEMENTS */
	QHash<int, QStringList> filters;

	for (int i = 0; i < m_queryElements.size(); i++) {

		int filterID        = m_queryElements.at(i).staticCast<SQLWhereElement>()->getFilterID();
		QString filterValue = m_queryElements.at(i).staticCast<SQLWhereElement>()->stringValue();

		if (m_queryElements.at(i)->elementType() == "where") {
			if (filters.contains(filterID)) {
				QStringList& tmp = filters[filterID];
				tmp.append(filterValue);
			} else {
				filters.insert(filterID, QStringList{ filterValue });
			}
		}
	}

	QHashIterator<int, QStringList> hash_iterator(filters);
	while (hash_iterator.hasNext()) {
		hash_iterator.next();
		QStringList tmpList = hash_iterator.value();
		if (whereApplied == false) {
			tmpQuery.append("WHERE ");
			if (tmpList.count() > 1) {
				tmpQuery.append("(");
				for (int i = 0; i < tmpList.size(); i++) {
					if (i != tmpList.size() - 1) {
						tmpQuery.append(tmpList.at(i) % " OR ");
					} else {
						tmpQuery.append(tmpList.at(i));
					}
				}
				tmpQuery.append(") ");
			} else {
				tmpQuery.append(" (" % hash_iterator.value().first() % ") ");
			}
			whereApplied = true;
		} else {
			if (tmpList.count() > 1) {
				tmpQuery.append("AND (");
				for (int i = 0; i < tmpList.size(); i++) {
					if (i < tmpList.size() - 1) {
						tmpQuery.append(tmpList.at(i) % " OR ");
					} else {
						tmpQuery.append(tmpList.at(i));
					}
				}
				tmpQuery.append(") ");
			} else {
				tmpQuery.append("AND (" % hash_iterator.value().first() % ") ");
			}
		}
	}

	/* UPDATE CONSTRAINS */
	if (getTableFromName(identityTable)) {
		if (getTableFromName(identityTable)->hasColumnAlias(identityColumn)) {
			QString tableAlias = getTableFromName(identityTable)->tableAlias();
			QString columnName = getTableFromName(identityTable)->getColumnName(identityColumn);
			if (whereApplied == true) {
				tmpQuery.append("AND " % tableAlias % "." % columnName % " IN (");
				for (auto constrain : elementsToRefresh) {
					tmpQuery.append(constrain.toString() % ", ");
				}
				tmpQuery.remove(tmpQuery.length() - 2, 2);
			} else {
				tmpQuery.append("WHERE " % tableAlias % "." % columnName % " IN (");
				for (auto constrain : elementsToRefresh) {
					tmpQuery.append(constrain.toString() % ", ");
				}
				tmpQuery.remove(tmpQuery.length() - 2, 2);
			}
			tmpQuery.append(")");
		} else {
			qCritical() << "Column:" << identityColumn << "does not exist for table:" << identityTable;
			return QString();
		}
	} else {
		qCritical() << "Table:" << identityTable << "does not exist.";
		return QString();
	}
	//	IzLogger::Logger::instance()->LOG(IzLogger::LogSeverity::DEBUG, "QUERY: " % tmpQuery);
	return tmpQuery;
}

void IzSQLUtilities::SQLQueryBuilder::clearQuery()
{
	/* query state */
	m_queryCorrupted = false;
	m_errorFound     = false;
	m_topApplied     = false;

	/* tables list */
	m_sqlTables.clear();

	/* query elements */
	m_queryElements.clear();
}

bool IzSQLUtilities::SQLQueryBuilder::tableExists(const QString& tableAlias, bool suppressWarnings) const
{
	for (int i = 0; i < m_sqlTables.size(); i++) {
		if (m_sqlTables.at(i)->tableAlias() == tableAlias) {
			return true;
		}
	}
	if (!suppressWarnings) {
		qCritical() << "Table with alias:" << tableAlias << "does not exist.";
	}
	return false;
}

void IzSQLUtilities::SQLQueryBuilder::invalidateQuery()
{
	m_queryCorrupted = true;
}

QSharedPointer<IzSQLUtilities::SQLTable> IzSQLUtilities::SQLQueryBuilder::getTableFromAlias(const QString& tableAlias) const
{
	if (m_sqlTables.isEmpty()) {
		qWarning() << "Tables list is empty.";
		return nullptr;
	} else {
		for (int i = 0; i < m_sqlTables.size(); i++) {
			if (m_sqlTables.at(i)->tableAlias() == tableAlias) {
				return m_sqlTables.at(i);
			}
		}
	}
	qCritical() << "Table with alias:" << tableAlias << "does not exist.";
	return nullptr;
}

QSharedPointer<IzSQLUtilities::SQLTable> IzSQLUtilities::SQLQueryBuilder::getTableFromName(const QString& tableName) const
{
	if (m_sqlTables.isEmpty()) {
		qWarning() << "Tables list is empty.";
		return nullptr;
	} else {
		for (int i = 0; i < m_sqlTables.size(); i++) {
			if (m_sqlTables.at(i)->tableName() == tableName) {
				return m_sqlTables.at(i);
			}
		}
	}
	qCritical() << "Table with name" << tableName << "does not exist.";
	return nullptr;
}

bool IzSQLUtilities::SQLQueryBuilder::sanityCheck(const QString& caller)
{
	if (m_queryCorrupted == true) {
		if (m_errorFound == false) {
			qCritical() << "Query is corrupted. Every next operation will silently fail. Caller:" << caller;
			m_errorFound = true;
		}
		return false;
	}
	return true;
}

void IzSQLUtilities::SQLQueryBuilder::select(const QVariantList& select)
{
	/* sanity check */
	if (!sanityCheck("select")) {
		return;
	}

	/* temporary variables we will use */
	QString selectType;
	QString tableAlias;
	QVariantList columns;

	foreach (QVariant tmpElement, select) {
		QVariantMap tmpSelectElement = tmpElement.toMap();

		/* check for necessary select parameters */
		if (!checkParameter(tmpSelectElement, "type") || !checkParameter(tmpSelectElement, "columns")) {
			qCritical() << "One of the select parameters is invalid. Aborting.";
			invalidateQuery();
			return;
		}

		/* check for parameters proper values */
		if (!checkParameterValue(pVal(tmpSelectElement, "type"), QStringList{ "table" })) {
			qCritical() << "Incorrect parameter value found. Aborting.";
			invalidateQuery();
			return;
		}

		/* check for proper table-related parameters */
		if (pVal(tmpSelectElement, "type") == "table") {

			if (!checkParameter(tmpSelectElement, "tableAlias")) {
				qCritical() << "'table' type specified but table alias parameter is invalid. Aborting.";
				invalidateQuery();
				return;
			}

			if (!tableExists(pVal(tmpSelectElement, "tableAlias"))) {
				qCritical() << "One of the requested tables does not exist. Aborting.";
				invalidateQuery();
				return;
			}

			tableAlias = pVal(tmpSelectElement, "tableAlias");
			selectType = pVal(tmpSelectElement, "selectType");

			/* column parameters parsing */
			foreach (QVariant tmpElement, tmpSelectElement.value("columns").toList()) {

				QVariantMap tmpColumnElement = tmpElement.toMap();

				/* check for parameters */
				if (!checkParameter(tmpColumnElement, "columnAlias")) {
					qCritical() << "Incorrect parameter value found. Aborting.";
					invalidateQuery();
					return;
				}

				/* check for existing columns */
				if (!getTableFromAlias(tableAlias)->hasColumnAlias(pVal(tmpColumnElement, "columnAlias"))) {
					qCritical() << "One of the requested columns does not exist. Aborting.";
					invalidateQuery();
					return;
				}
			}
		}

		columns = tmpSelectElement.value("columns").toList();

		/* create select elements */
		foreach (QVariant element, columns) {

			QVariantMap column = element.toMap();
			bool isDuplicate   = false;

			/* check for existing select elements */
			/* duplicate = there is already defined select element with this column alias */
			for (int i = 0; i < m_queryElements.size(); i++) {
				if (m_queryElements.at(i)->elementType() == "select" && m_queryElements.at(i).staticCast<SQLSelectElement>()->columnAlias() == pVal(column, "columnAlias")) {
					qWarning() << "Column with alias:" << pVal(column, "columnAlias") << "already selected. Skipping.";
					isDuplicate = true;
				}
			}
			if (!isDuplicate) {
				QSharedPointer<SQLElement> selectElement(new SQLSelectElement(selectType, tableAlias, pVal(column, "columnAlias"), getTableFromAlias(tableAlias)->getColumnName(pVal(column, "columnAlias")), pVal(column, "coalesce")));
				selectElement->setParentTable(getTableFromAlias(tableAlias));
				m_queryElements.append(selectElement);
			}
		}
	}
}

bool IzSQLUtilities::SQLQueryBuilder::queryIsValid() const
{
	if (m_queryCorrupted) {
		qCritical() << "Query is corrupted.";
		return false;
	}
	if (m_sqlTables.empty()) {
		qWarning() << "No tables were added to the query.";
		return false;
	}
	int counter = 0;
	for (int i = 0; i < m_queryElements.size(); i++) {
		if (m_queryElements.at(i)->elementType() == QStringLiteral("select")) {
			counter++;
		}
	}
	if (counter == 0) {
		qWarning() << "No columns selected.";
		return false;
	}
	return true;
}

bool IzSQLUtilities::SQLQueryBuilder::checkParameterValue(const QString& parameterValue, const QStringList& expectedValues)
{
	if (expectedValues.contains(parameterValue)) {
		return true;
	}
	qCritical() << "Value:" << parameterValue << "is invalid.";
	return false;
}
