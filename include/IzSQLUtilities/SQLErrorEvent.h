#ifndef IZSQLUTILITIES_SQLERROREVENT_H
#define IZSQLUTILITIES_SQLERROREVENT_H

#include <QCoreApplication>
#include <QEvent>
#include <QSqlError>

namespace IzSQLUtilities
{

	class SQLErrorEvent : public QEvent
	{
	public:
		// ctor
		explicit SQLErrorEvent(const QSqlError& sqlError)
			: m_sqlError(sqlError)
			, QEvent(m_eventType)
		{
		}

		// static post event function
		static void postSQLError(const QSqlError& sqlError)
		{
			QCoreApplication::postEvent(dynamic_cast<QObject*>(QCoreApplication::instance()), new SQLErrorEvent(sqlError));
		}

		// m_eventType getter
		static QEvent::Type eventType()
		{
			return m_eventType;
		}

		// m_sqlError getter
		const QSqlError& sqlError() const
		{
			return m_sqlError;
		}

		// TODO: do not initialize with hardcoded value
		static const QEvent::Type m_eventType = static_cast<QEvent::Type>(43256);

	private:
		// sql error associated with this event
		const QSqlError m_sqlError;
	};
}   // namespace IzSQLUtilities

#endif   // SQLERROREVENT_H
