#ifndef IZSQLUTILITIES_SQLQUERYBUILDER_H
#define IZSQLUTILITIES_SQLQUERYBUILDER_H

/*
	SQL Query Builder

	TODO: różne typy WHERE !=, =, IS, IS NOT
	TODO: uzupełnić QML Syntax dla joina()
	TODO: uzupełnić QML Syntax dla select()
	TODO: uzupełnić QML Syntax dla addTable()
	TODO: we wszystkich funkcjach sprawdzać naddatkowe parametry
	TODO: w getQuery() dodać sprawdzanie czy dane parametry rzeczywiście istnieją
	TODO: więcej typów w select()
	TODO: przy usuwaniu tablicy uswać też powiązane joiny
	TODO: w select() sprawdzać coalesce
	TODO: cała internalData() do przepisania jest
	TODO: w getUpdateQuery() sprawdzac alias i kolumnę
*/

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QHash>
#include <QObject>
#include <QSharedPointer>
#include <QVariant>

namespace IzSQLUtilities
{
	class SQLTable;
	class SQLElement;

	class IZSQLUTILITIESSHARED_EXPORT SQLQueryBuilder : public QObject
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLQueryBuilder)

	public:
		// ctor
		explicit SQLQueryBuilder(QObject* parent = nullptr);

		// VIEW RELATED FUNCTIONS

		// sets the view for use in query
		Q_INVOKABLE void setView(const QString& query, const QString& orderBy = "");

		// returns constructed view
		Q_INVOKABLE const QString getView();

		// clears ciew related data
		Q_INVOKABLE void clearView();

		// adds filter to the view
		Q_INVOKABLE void addViewFilter(int filterID, const QVariantMap& filterDefinition);

		// removes filter from the view
		Q_INVOKABLE void removeViewFilter(int filterID);

		// _viewIsInitialized getter
		Q_INVOKABLE bool getViewIsInitialized() const;

		// VIEW RELATED FUNCTIONS END

		// DYNAMIC QUERY RELATED FUNCTIONS

		// adds table to the query
		/*
			QML Syntax
			{
				{"tableName": "dbo.tableName";"tableAlias": "_TN"},
				{"columnAlias_1": "columnName","columnAlias_2": "columnName"}
			}
		*/
		Q_INVOKABLE void addTable(const QVariantMap& tableDefinition, const QVariantList& tableColumns);

		// removes table from the query
		Q_INVOKABLE void removeTable(const QVariantMap& tableDefinition);

		// joins two tables
		/*
			QML Syntax
			{
				{"table_1",."table_2"},
				{}
			}
		*/
		Q_INVOKABLE void join(const QVariantMap& tables, const QVariantMap& joinParameters);

		// sets top for query, when called twice old value is replaced
		/*
			QML Syntax
			{
				{"value":100, "type": "[integer, percentage]"}
			}
		*/
		Q_INVOKABLE void top(const QVariantMap& top);

		// removes top element from query
		Q_INVOKABLE void removeTop();

		// sets order for query
		Q_INVOKABLE void orderBy(const QVariantList& order);

		// adds filter to the query
		Q_INVOKABLE void addFilter(const QVariantMap& filterDefinition);

		// removes filter from the query
		Q_INVOKABLE void removeFilter(const QVariantMap& filterDefinition);

		// changes filter value
		Q_INVOKABLE void changeFilter(const QVariantMap& filterDefinition);

		// returns constructed query or Null QString if query is invalid
		Q_INVOKABLE const QString getQuery();

		// returns constructed count expression for current query or Null QString if query is invalid
		Q_INVOKABLE const QString getCountQuery();

		// returns query used to partially refresh elements in model or Null QString if query is invalid*/
		const QString getPartialQuery(const QString& identityColumn, const QString& identityTable, const QVariantList& elementsToRefresh);

		// resets all internal data
		Q_INVOKABLE void clearQuery();

		// adds element to the select list
		/*
			QML Syntax
			{
				{"type": [table, {}], "tableAlias": "alias", "columns":[{"columnAlias": "alias", "coalesce": "1"}]}
			}
		*/
		Q_INVOKABLE void select(const QVariantList& select);

		// query is considered valid if:
		// 1. at least one table was added to it
		// 2. at least one select element was defined
		// 3. there were no errors during query creation [_queryCorrupted = false]
		Q_INVOKABLE bool queryIsValid() const;

		// DYNAMIC QUERY RELATED FUNCTIONS END

	private:
		// returns parameter value from QVariantMap
		const QString pVal(const QVariantMap& parameters, const QString& parameter) const;

		// checks for proper parameter signature
		bool checkParameter(const QVariantMap& parameters, const QString& parameter) const;

		// checks for proper parameter values
		bool checkParameterValue(const QString& parameterValue, const QStringList& expectedValues);

		// checks if the requested table is properly added to the tables list
		bool tableExists(const QString& tableAlias, bool suppressWarnings = false) const;

		// invalidates current query
		void invalidateQuery();

		// view definition used in the query
		QString m_view;

		// order by string used by the query
		QString m_viewOrderBy;

		// filters used by the view query
		QHash<int, QString> m_viewFilters;

		// true if view was set
		bool m_viewIsInitialized;

		// true if one of the query operations criticaly failed
		bool m_queryCorrupted;

		// used to suppress spam from query functions when one of them reported error
		bool m_errorFound;

		// true if top was applied to query
		bool m_topApplied;

		// returns table with given alias or nullptr if no table was found
		QSharedPointer<SQLTable> getTableFromAlias(const QString& tableAlias) const;

		// returns table with given name or nullptr if no table was found
		QSharedPointer<SQLTable> getTableFromName(const QString& tableName) const;

		// function used to check if next operation can be completed, returns false if query is corrupted
		bool sanityCheck(const QString& caller);

		// list of sql tables used in query
		QList<QSharedPointer<SQLTable>> m_sqlTables;

		// query elements used by the query
		QList<QSharedPointer<SQLElement>> m_queryElements;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLQUERYBUILDER_H
