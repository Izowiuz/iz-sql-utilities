#include "IzSQLUtilities/SQLdbc.h"

#include <QCoreApplication>
#include <QDebug>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

#include "IzSQLUtilities/SQLErrorInterpreterA2.h"

IzSQLUtilities::SQLdbc::SQLdbc(const QString& connectionSignature, bool globalParameters, DatabaseType databaseType, const QVariantMap& databaseParameters, QObject* parent)
	: QObject(parent)
	, m_databaseType(databaseType)
	, m_globalParameters(globalParameters)
	, m_databaseParameters(databaseParameters)
{
	// QCoreApplication handler
	QCoreApplication* appHandler = QCoreApplication::instance();

	if (appHandler != nullptr) {
		// optional global parameters
		m_userLogin = appHandler->property("SQLdbc_appUserLogin").toString();
		if (m_userLogin.isEmpty()) {
			qInfo() << "Could not find defined application user login for the connection mark purposes. It will be set to: SQLdbc";
		}

		m_appUserID = appHandler->property("SQLdbc_appUserID").toString();
		if (m_appUserID.isEmpty()) {
			qInfo() << "Could not find defined application user ID for the connection mark purposes. It will be set to: 0";
		}

		m_applicationName = appHandler->property("SQLdbc_appName").toString();
		if (m_applicationName.isEmpty()) {
			qInfo() << "Could not find defined application name for the connection mark purposes. It will be set to: app";
			m_applicationName = QStringLiteral("app");
		}

		// required global parameters
		if (m_globalParameters) {
			m_serverName   = appHandler->property("SQLdbc_server").toString();
			m_databaseName = appHandler->property("SQLdbc_database").toString();
			m_databaseUser = appHandler->property("SQLdbc_dbUserLogin").toString();
			m_userPassword = appHandler->property("SQLdbc_dbUserPassword").toString();
			m_localDBPath  = appHandler->property("SQLdbc_sqlitePath").toString();

			// parameters check
			if (m_serverName.isEmpty() || m_databaseName.isEmpty() || m_databaseUser.isEmpty() || m_userPassword.isEmpty()) {
				qWarning() << "Default parameters requested but one of them is invalid.";
			} else {
				m_connectionInitialized = true;
			}
		}
	} else {
		qWarning() << "QCoreApplication::instance() returned NULL. Application user login for the connection mark purposes will be set to: SQLdbc";
		qWarning() << "To properly initialize setupConnection() has to be used.";
	}

	// generated connection identifier
	m_connectionUUID = m_userLogin + QStringLiteral("-") + QUuid::createUuid().toString() + QStringLiteral("-") + connectionSignature;
}

IzSQLUtilities::SQLdbc::~SQLdbc()
{
	if (m_connectionOpened) {
		closeConnection();
	}
}

void IzSQLUtilities::SQLdbc::setupConnection(const QString& server, const QString& database, const QString& userLogin, const QString& userPassword, const QString& localDBPath)
{
	m_serverName            = server;
	m_databaseName          = database;
	m_databaseUser          = userLogin;
	m_userPassword          = userPassword;
	m_localDBPath           = localDBPath;
	m_connectionInitialized = true;
}

bool IzSQLUtilities::SQLdbc::initializeConnection(const QStringList& connectionParameters)
{
	switch (m_databaseType) {
	case IzSQLUtilities::DatabaseType::MSSQL:
		return initializeMSSQLConnection(connectionParameters);
	case IzSQLUtilities::DatabaseType::SQLITE:
		return initializeSQLiteConnection(connectionParameters);
	}
	return false;
}

const QString IzSQLUtilities::SQLdbc::getConnectionName() const
{
	if (!m_connectionOpened) {
		qCritical() << "Connection was not opened.";
	}
	return m_connectionName;
}

QSqlDatabase IzSQLUtilities::SQLdbc::getConnection() const
{
	if (!m_connectionOpened) {
		qWarning() << "Connection handler requested but connection is not initialized.";
	}
	return QSqlDatabase::database(m_connectionName, false);
}

void IzSQLUtilities::SQLdbc::closeConnection()
{
	if (m_connectionOpened) {
		{
			QSqlDatabase db = getConnection();
			if (db.isOpen()) {
				db.close();
				qInfo() << "Connection:" << m_connectionName << "closed.";
			} else {
				qWarning() << "Attempting to close:" << m_connectionName << "but connection is not opened.";
			}
		}
		QSqlDatabase::removeDatabase(m_connectionName);
	} else {
		qWarning() << "Attempting to close connection but connection is not initialized.";
	}
}

QSqlError IzSQLUtilities::SQLdbc::lastError() const
{
	return QSqlDatabase::database(m_connectionName, false).lastError();
}

bool IzSQLUtilities::SQLdbc::initializeMSSQLConnection(const QStringList& connectionParameters)
{
	if (m_connectionInitialized) {
		m_connectionName = m_databaseName + QStringLiteral("-") + m_connectionUUID;
		{
			QSqlDatabase sql = QSqlDatabase::addDatabase(QStringLiteral("QODBC"), m_connectionName);

			// connection parameters
			QString cParameters;
			for (const auto& parameter : qAsConst(connectionParameters)) {
				cParameters += parameter + QStringLiteral(";");
			}
			sql.setConnectOptions(cParameters);

			// opening the connection
			QString connectionTemplate;
			for (const auto& driver : qAsConst(m_mssqlDrivers)) {
				connectionTemplate = QStringLiteral("Driver=%1;Server=%2;Database=%3;Uid=%4;Pwd=%5;app=%6").arg(driver, m_serverName, m_databaseName, m_databaseUser, m_userPassword, m_applicationName + QStringLiteral("@") + m_appUserID);
				sql.setDatabaseName(connectionTemplate);
				if (sql.open()) {
					qInfo() << "Connection:" << m_connectionName << "opened with" << driver;
					m_connectionOpened = true;
					break;
				}
				qWarning() << "Error opening database connection with:" << driver;
			}
		}
		if (!m_connectionOpened) {
			QSqlDatabase::removeDatabase(m_connectionName);
			qCritical() << lastError().text();
			if (connectionParameters.empty()) {
				SQLErrorInterpreterA2::instance()->sqlResponse(SQLResponseSeverity::SQL_RESPONSE_ERROR, lastError().text());
			}
		}
		return m_connectionOpened;
	}
	qWarning() << "Attempting to initialize connection with custom parameters but they are not provided.";
	return false;
}

bool IzSQLUtilities::SQLdbc::initializeSQLiteConnection(const QStringList& connectionParameters)
{
	Q_UNUSED(connectionParameters)

	if (!m_localDBPath.isNull() && !m_localDBPath.isEmpty()) {
		{
			m_connectionName = QStringLiteral("local-") + m_connectionUUID;
			QSqlDatabase sql = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
			sql.setDatabaseName(m_localDBPath + QStringLiteral("/localdb.sqlite"));
			if (sql.open()) {
				qInfo() << "Local SQLite connection:" << m_connectionName << "opened.";
				m_connectionOpened = true;
			} else {
				qCritical() << "Error opening local SQLite database connection.";
				qCritical() << lastError().text();
				if (connectionParameters.empty()) {
					SQLErrorInterpreterA2::instance()->sqlResponse(SQLResponseSeverity::SQL_RESPONSE_ERROR, lastError().text());
				}
			}
		}
		if (!m_connectionOpened) {
			QSqlDatabase::removeDatabase(m_connectionName);
		}
	} else {
		m_connectionOpened = false;
	}
	return m_connectionOpened;
}
