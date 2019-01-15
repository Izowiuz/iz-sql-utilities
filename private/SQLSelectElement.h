#ifndef IZSQLUTILITIES_SQLSELECTELEMENT_H
#define IZSQLUTILITIES_SQLSELECTELEMENT_H

#include "SQLElement.h"

namespace IzSQLUtilities
{
	class SQLSelectElement : public SQLElement
	{
	public:
		// ctor
		explicit SQLSelectElement(const QString& selectType,
								  const QString& tableAlias,
								  const QString& columnAlias,
								  const QString& columnName,
								  const QString& coalesce);

		virtual ~SQLSelectElement() = default;

		// SQLELement interface
		const QString stringValue() const override;

		// SQLELement interface
		const QString elementType() const override;

		// _columnAlias getter
		QString columnAlias() const;

	private:
		// element parameters
		QString m_selectType;
		QString m_tableAlias;
		QString m_columnAlias;
		QString m_columnName;
		QString m_coalesce;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLSELECTELEMENT_H
