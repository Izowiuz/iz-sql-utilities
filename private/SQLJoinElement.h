#ifndef IZSQLUTILITIES_SQLJOINELEMENT_H
#define IZSQLUTILITIES_SQLJOINELEMENT_H

#include "SQLElement.h"

namespace IzSQLUtilities
{
	class SQLJoinElement : public SQLElement
	{
	public:
		// ctor
		explicit SQLJoinElement(const QString& tableNanme,
								const QString& leftTable,
								const QString& rightTable,
								const QString& leftConstrain,
								const QString& rightConstrain,
								const QString& joinType,
								const QList<QStringList>& andSubelements);

		virtual ~SQLJoinElement() = default;

		// SQLELement interface
		const QString stringValue() const override;

		// SQLELement interface
		const QString elementType() const override;

	private:
		// element parameters
		QString m_tableName;
		QString m_leftTable;
		QString m_rightTable;
		QString m_leftConstrain;
		QString m_rightConstrain;
		QString m_joinType;
		QList<QStringList> m_andSubelements;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLJOINELEMENT_H
