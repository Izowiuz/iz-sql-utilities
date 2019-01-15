#include "SQLErrorMessage.h"

IzSQLUtilities::SQLErrorMessage::SQLErrorMessage(const QString& errorTitle,
												 const QString& errorDescription,
												 const QString& actionToTake,
												 const QString& sqlError,
												 QObject* parent)
	: QObject(parent)
	, m_errorTitle(errorTitle)
	, m_errorDescription(errorDescription)
	, m_actionToTake(actionToTake)
	, m_sqlError(sqlError)
{
}

QString IzSQLUtilities::SQLErrorMessage::errorTitle() const
{
	return m_errorTitle;
}

QString IzSQLUtilities::SQLErrorMessage::errorDescription() const
{
	return m_errorDescription;
}

QString IzSQLUtilities::SQLErrorMessage::actionToTake() const
{
	return m_actionToTake;
}

QString IzSQLUtilities::SQLErrorMessage::sqlError() const
{
	return m_sqlError;
}
