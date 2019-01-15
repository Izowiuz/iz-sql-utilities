#ifndef IZSQLUTILITIES_SQLRESPONSEEVENT_H
#define IZSQLUTILITIES_SQLRESPONSEEVENT_H

#include <QEvent>
#include <QSqlError>

// TODO: po co to było ... ?
namespace IzSQLUtilities
{
	class SQLResponseEvent : public QEvent
	{
	public:
		// ctor
		explicit SQLResponseEvent(QEvent::Type type,
								  const QSqlError& error);

		// error getter
		QSqlError getError() const;

	private:
		// response error
		QSqlError m_error;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLRESPONSEEVENT_H
