#ifndef IZSQLUTILITIES_SQLERRORINTERPRETERA2_H
#define IZSQLUTILITIES_SQLERRORINTERPRETERA2_H

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QHash>
#include <QObject>
#include <QSqlError>
#include <QVariantMap>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"

class QQmlApplicationEngine;

namespace IzSQLUtilities
{
	class IZSQLUTILITIESSHARED_EXPORT SQLErrorInterpreterA2 : public QObject
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLErrorInterpreterA2)

	public:
		// ctor
		explicit SQLErrorInterpreterA2(QObject* parent = nullptr);

		// static instance getter
		static SQLErrorInterpreterA2* instance();

		// used to reqister interpreter in the QML engine context
		void registerInQMLEngine(QQmlApplicationEngine* engine, const QString& name = QString());

		// used to send sql response event
		void sqlResponse(SQLResponseSeverity severity, const QSqlError& error);

		// adds custom error to the error interpreter
		// returns true on success and false otherwise
		bool addErrorDefinition(int errorNumber, const QString& errorDescription);

	private:
		// true if interpreter was registred in qml engine
		bool m_registredInQMLEngine{ false };

		// used to interprete sql error
		void interpretError(const QSqlError& error);

		// holds error numbers with their descriptions
		QHash<int, QString> m_customSQLErrors;

	protected:
		// QObject interface
		void customEvent(QEvent* event) override;

	signals:
		void sqlError(QVariantMap error);
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLERRORINTERPRETERA2_H
