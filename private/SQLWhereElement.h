#ifndef IZSQLUTILITIES_SQLWHEREELEMENT_H
#define IZSQLUTILITIES_SQLWHEREELEMENT_H

#include "SQLElement.h"

namespace IzSQLUtilities
{
	class SQLWhereElement : public SQLElement
	{
	public:
		// ctor
		explicit SQLWhereElement(const QString& tableAlias,
								 const QString& columnName,
								 const QString& whereValue);

		virtual ~SQLWhereElement() = default;

		// SQLELement interface
		const QString stringValue() const override;

		// SQLELement interface
		const QString elementType() const override;

		// _tableAlias getter
		QString tableAlias() const;

		// _columnName getter
		QString columnName() const;

		// _whereValue getter / setter
		QString whereValue() const;
		void setWhereValue(const QString& whereValue);

		// _isDynamic getter / setter
		bool isDynamic() const;
		void setIsDynamic(bool isDynamic);

		// _filterID getter / setter
		int getFilterID() const;
		void setFilterID(int value);

	private:
		// element parameters
		QString m_tableAlias;
		QString m_columnName;
		QString m_whereValue;
		bool m_isDynamic{ false };
		int m_filterID{ 0 };
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLWHEREELEMENT_H
