#ifndef IZSQLUTILITIES_SQLERRORMESSAGE_H
#define IZSQLUTILITIES_SQLERRORMESSAGE_H

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QObject>

namespace IzSQLUtilities
{
	class IZSQLUTILITIESSHARED_EXPORT SQLErrorMessage : public QObject
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLErrorMessage)

	public:
		// ctor
		Q_INVOKABLE explicit SQLErrorMessage(const QString& errorTitle,
											 const QString& errorDescription,
											 const QString& actionToTake,
											 const QString& sqlError,
											 QObject* parent = nullptr);

		// error parameters getters
		Q_INVOKABLE QString errorTitle() const;
		Q_INVOKABLE QString errorDescription() const;
		Q_INVOKABLE QString actionToTake() const;
		Q_INVOKABLE QString sqlError() const;

	private:
		// error parameters
		QString m_errorTitle;
		QString m_errorDescription;
		QString m_actionToTake;
		QString m_sqlError;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLERRORMESSAGE_H
