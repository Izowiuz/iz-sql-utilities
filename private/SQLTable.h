#ifndef IZSQLUTILITIES_SQLTABLE_H
#define IZSQLUTILITIES_SQLTABLE_H

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QHash>

namespace IzSQLUtilities
{
	class IZSQLUTILITIESSHARED_EXPORT SQLTable
	{
	public:
		// ctor
		explicit SQLTable(const QString& tableName,
						  const QString& tableAlias);

		// name of the table
		QString tableName() const;
		void setTableName(const QString& tableName);

		// alias of the table
		QString tableAlias() const;
		void setTableAlias(const QString& tableAlias);

		// adds table column, checks for duplicates
		bool addTableColumn(const QString& columnAlias, const QString& columnName);

		// returns all table columns
		QHash<QString, QString> tableColumns() const;

		// returns true if table contains given column alias
		bool hasColumnAlias(const QString& columnAlias) const;

		// returns true if table contains given column
		bool hasColumn(const QString& columnName) const;

		// returns column name for given column alias
		const QString getColumnName(const QString& columnAlias) const;

	private:
		// name of the table
		QString m_tableName;

		// alias of the table
		QString m_tableAlias;

		// column alias | column name*/
		QHash<QString, QString> m_tableColumns;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLTABLE_H
