#ifndef IZSQLUTILITIES_SQLTOPELEMENT_H
#define IZSQLUTILITIES_SQLTOPELEMENT_H

#include "SQLElement.h"

namespace IzSQLUtilities
{
	class SQLTopElement : public SQLElement
	{
	public:
		// ctor
		explicit SQLTopElement(const QString& type,
							   const QString& value);

		virtual ~SQLTopElement() = default;

		// SQLELement interface
		const QString stringValue() const override;

		// SQLELement interface
		const QString elementType() const override;

		// value getter
		QString value() const;

	private:
		// element parameters
		QString m_type;
		QString m_value;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLTOPELEMENT_H
