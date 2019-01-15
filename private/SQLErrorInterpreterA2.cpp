#include "IzSQLUtilities/SQLErrorInterpreterA2.h"

#include <QCoreApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "SQLResponseEvent.h"

Q_GLOBAL_STATIC(IzSQLUtilities::SQLErrorInterpreterA2, interpreterinstance);

IzSQLUtilities::SQLErrorInterpreterA2::SQLErrorInterpreterA2(QObject* parent)
	: QObject(parent)
{
}

IzSQLUtilities::SQLErrorInterpreterA2* IzSQLUtilities::SQLErrorInterpreterA2::instance()
{
	if (interpreterinstance->thread() != QCoreApplication::instance()->thread()) {
		interpreterinstance->moveToThread(QCoreApplication::instance()->thread());
	}
	return interpreterinstance;
}

void IzSQLUtilities::SQLErrorInterpreterA2::registerInQMLEngine(QQmlApplicationEngine* engine, const QString& name)
{
	if (engine != nullptr) {
		if (name.isNull()) {
			engine->rootContext()->setContextProperty(QStringLiteral("sei"), instance());
			qInfo() << "Class registered in QML engine under default name 'sei'.";
		} else {
			engine->rootContext()->setContextProperty(name, instance());
			qInfo() << "Class registered in QML engine under name:" << name;
		}
		m_registredInQMLEngine = true;
	} else {
		qInfo() << "An error occurred. Class is not registred in QML engine.";
	}
}

void IzSQLUtilities::SQLErrorInterpreterA2::sqlResponse(IzSQLUtilities::SQLResponseSeverity severity, const QSqlError& error)
{
	QCoreApplication::postEvent(instance(), new SQLResponseEvent(static_cast<QEvent::Type>(severity), error));
}

bool IzSQLUtilities::SQLErrorInterpreterA2::addErrorDefinition(int errorNumber, const QString& errorDescription)
{
	if (!m_customSQLErrors.contains(errorNumber)) {
		m_customSQLErrors.insert(errorNumber, errorDescription);
		return true;
	}
	qWarning() << "Error with number:" << errorNumber << "is already defined as:" << errorNumber;
	return false;
}

void IzSQLUtilities::SQLErrorInterpreterA2::interpretError(const QSqlError& error)
{
	int errorNumber = error.number();
	QString errorTitle;
	QString errorDescription;
	QString actionToTake;
	QString dbSqlError;

	/* in MSSQL, error numbers lower than 50000 are system messages */
	if (errorNumber > 50000) {
		if (m_customSQLErrors.contains(errorNumber)) {
			errorTitle       = QStringLiteral("Wystąpił błąd bazy danych");
			errorDescription = QStringLiteral("Silnik bazodanowy zwrócił kod błędu: ") + QString::number(errorNumber);
			actionToTake     = QStringLiteral("Skontaktuj się z działem IT. Jeżeli zostaniesz poproszony o szczegóły podaj tekst poniżej.");
			dbSqlError       = m_customSQLErrors.value(errorNumber);
		} else {
			errorTitle       = QStringLiteral("Wystąpił nieokreśony błąd bazydanych");
			errorDescription = QStringLiteral("Wystąpił nieobsługiwany błąd bazy danych");
			actionToTake     = QStringLiteral("Skontaktuj się z działem IT. Jeżeli zostaniesz poproszony o szczegóły podaj tekst poniżej.");
			if (!error.text().isEmpty()) {
				dbSqlError = error.text();
			} else {
				dbSqlError = QStringLiteral("Silnik bazodanowy nie zwrócił komunikatu błędu.");
			}
		}
	} else {
		errorTitle       = QStringLiteral("Wystąpił błąd bazy danych");
		errorDescription = QStringLiteral("Silnik bazodanowy zwrócił kod błędu: ") + QString::number(errorNumber);
		actionToTake     = QStringLiteral("Skontaktuj się z działem IT. Jeżeli zostaniesz poproszony o szczegóły podaj tekst poniżej.");
		if (!error.text().isEmpty()) {
			dbSqlError = error.text();
		} else {
			dbSqlError = QStringLiteral("Silnik bazodanowy nie zwrócił komunikatu błędu.");
		}
	}
	emit sqlError(QVariantMap{
		{ QStringLiteral("errorTitle"), errorTitle },
		{ QStringLiteral("errorDescription"), errorDescription },
		{ QStringLiteral("actionToTake"), actionToTake },
		{ QStringLiteral("sqlError"), dbSqlError } });
}

void IzSQLUtilities::SQLErrorInterpreterA2::customEvent(QEvent* event)
{
	if (auto e = static_cast<SQLResponseEvent*>(event)) {
		interpretError(e->getError());
	}
}
