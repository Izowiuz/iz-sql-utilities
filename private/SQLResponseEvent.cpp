#include "SQLResponseEvent.h"

IzSQLUtilities::SQLResponseEvent::SQLResponseEvent(QEvent::Type type,
												   const QSqlError& error)
	: QEvent(type)
	, m_error(error)
{
}

QSqlError IzSQLUtilities::SQLResponseEvent::getError() const
{
	return m_error;
}
