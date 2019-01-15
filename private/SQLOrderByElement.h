#ifndef IZSQLUTILITIES_SQLORDERBYELEMENT_H
#define IZSQLUTILITIES_SQLORDERBYELEMENT_H

#include "SQLElement.h"

namespace IzSQLUtilities
{
	class SQLOrderByElement : public SQLElement
	{
	public:
		// ctor
		explicit SQLOrderByElement(const QString& tableAlias,
								   const QString& tableColumn,
								   const QString& orderType);

		virtual ~SQLOrderByElement() = default;

		// SQLELement interface
		const QString stringValue() const override;

		// SQLELement interface
		const QString elementType() const override;

		// _tableAlias getter
		QString tableAlias() const;

		// _tableColumn getter
		QString tableColumn() const;

	private:
		// element parameters
		QString m_tableAlias;
		QString m_tableColumn;
		QString m_orderType;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLORDERBYELEMENT_H
