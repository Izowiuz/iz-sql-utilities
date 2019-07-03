#ifndef IZSQLUTILITIES_SQLDBC_H
#define IZSQLUTILITIES_SQLDBC_H

/*
	SQL Database Connector

	Connection procedure goes as follows:
		1.	if the defaultParameters is set to false the SQLdbc expects connection parameters to be supplied
			by the setupConnection function before connection can be initialized with the initializeConnection()
		2.	if dP is set to true it will try, firstly, to initialize connection parameters from the global
			QCoreApplication QProperties and, if this will fail for any reason, it will then fall back
			to the predefinded connection - mostly for the compatibility reasons

	SQLdbc expects parameters:
		SQLdbc_server
		SQLdbc_database
		SQLdbc_dbUserLogin
		SQLdbc_dbUserPassword
		SQLdbc_appUserLogin
		SQLdbc_appUserID
		SQLdbc_appName
		SQLdbc_sqlitePath

	TODO: coś nie tak z usuwaniem połączenia jeżeli jeden z driverów nie jest dostępny na maszynie
	TODO: ogarnać sytuację z databaseParameters, setupConnection i initializeConnection
*/

#include <QMap>
#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QVariant>
#include <QVector>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"
#include "IzSQLUtilities/IzSQLUtilities_Global.h"

namespace IzSQLUtilities
{
	class IZSQLUTILITIESSHARED_EXPORT SQLdbc : public QObject
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLdbc)

	public:
		// ctors
		// when globalParameters is set to false setupConnection has to be used to initialize connection parameters
		explicit SQLdbc(const QString& connectionSignature        = {},
						bool globalParameters                     = true,
						IzSQLUtilities::DatabaseType databaseType = IzSQLUtilities::DatabaseType::MSSQL,
						const QVariantMap& databaseParameters     = {},
						QObject* parent                           = nullptr);

		// dtor - closses connection on object destruction
		~SQLdbc();

		// initializes connection required database parameters
		// important only when globalParameters is set to false
		void setupConnection(const QString& server,
							 const QString& database,
							 const QString& userLogin,
							 const QString& userPassword,
							 const QString& localDBPath = {});

		// initializes connection
		bool initializeConnection(const QStringList& connectionParameters = {});

		// returns current connection name
		const QString getConnectionName() const;

		// returns connection associated with the current SQLdbc object
		QSqlDatabase getConnection() const;

		// closes initialized connection, removes DB name from connection list
		void closeConnection();

		// returns last error associated with current connection
		QSqlError lastError() const;

	private:
		// MSSQL db connection initialization
		bool initializeMSSQLConnection(const QStringList& connectionParameters = {});

		// SQLite connection initialization
		bool initializeSQLiteConnection(const QStringList& connectionParameters = {});

		// database type set for this connection
		IzSQLUtilities::DatabaseType m_databaseType;

		// database parameters passed in SQLdbc constructor
		const QVariantMap m_databaseParameters;

		// true if sql.open() returned true and false otherwise
		bool m_connectionOpened{ false };

		// true if connection parameters were set up
		bool m_connectionInitialized{ false };

		// if set to true SQLdbc will try to get db parameters from QCoreApplication instance
		bool m_globalParameters{ false };

		// ======================= base database parameters ===========================

		// server name
		QString m_serverName;

		// database name
		QString m_databaseName;

		// database user
		QString m_databaseUser;

		// user password
		QString m_userPassword;

		// path to the local database
		// with db name eg. "some/path/sqlite.db"
		QString m_localDBPath;

		// ======================= base database parameters ===========================

		// ======================= additional sqldbc parameters =======================

		// application user id
		QString m_appUserID = QStringLiteral("0");

		// application name
		QString m_applicationName = QStringLiteral("app");

		// application user - used to mark the connection
		QString m_userLogin = QStringLiteral("SQLdbc");

		// ======================= additional sqldbc parameters =======================

		QVector<QString> m_mssqlDrivers{
			QStringLiteral("{SQL Server Native Client 10.0}"),
			QStringLiteral("{SQL Server Native Client 11.0}")
		};

		// autognerated from UUID: database - m_connectionUUID
		QString m_connectionName;

		// generated UUID of the connection [userLogin - QUuid - connectionSignature]
		QString m_connectionUUID;
	};
}   // namespace IzSQLUtilities

#endif   // SQLDBC_H
