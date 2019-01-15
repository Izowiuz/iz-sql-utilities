#ifndef IZSQLUTILITIES_SQLELEMENT_H
#define IZSQLUTILITIES_SQLELEMENT_H

#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QVariantList>

namespace IzSQLUtilities
{
	class SQLTable;

	class SQLElement
	{
	public:
		// ctor
		explicit SQLElement() = default;

		// dtor
		virtual ~SQLElement() = default;

		// returns string value for this element
		virtual const QString stringValue() const = 0;

		// returns type for this query element
		virtual const QString elementType() const = 0;

		// _parentTable setter / getter
		QSharedPointer<SQLTable> parentTable() const;
		void setParentTable(QSharedPointer<SQLTable> parentTable);

	private:
		// holds table to which this element belongs to
		QSharedPointer<SQLTable> m_parentTable{ nullptr };
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLELEMENT_H
