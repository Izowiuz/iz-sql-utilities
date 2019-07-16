#ifndef IZSQLUTILITIES_SQLCONNECTOR_H
#define IZSQLUTILITIES_SQLCONNECTOR_H

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QUuid>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"
#include "IzSQLUtilities_Global.h"

namespace IzSQLUtilities
{
	class SqlConnector
	{
	public:
		explicit SqlConnector(DatabaseType databaseType, const QVariantMap& connectionParameters = {})
			: m_databaseType(databaseType)
		{
			switch (m_databaseType) {
			case DatabaseType::MSSQL:
				initializeMSSQLDB(connectionParameters);
				break;
			case DatabaseType::PSQL:
				initializePSQLDB(connectionParameters);
				break;
			case DatabaseType::SQLITE:
				initializeSQLITEDB(connectionParameters);
				break;
			}
		}

		// dtor - closses non-pooled connection on object destruction
		~SqlConnector()
		{
			closeConnection();
		}

		SqlConnector(const SqlConnector& other) = delete;
		SqlConnector(SqlConnector&& other)      = delete;

		// returns created connection type
		IzSQLUtilities::DatabaseType connectionType() const
		{
			return m_databaseType;
		}

		// returns created database connection
		QSqlDatabase getConnection() const
		{
			return m_database;
		}

		// returns last error generated for this connection
		QSqlError lastError() const
		{
			return m_database.lastError();
		}

		// returns generated connection name
		QString getConnectionName() const
		{
			return m_connectionName;
		}

		// closes generated connection
		void closeConnection()
		{
			if (m_database.isOpen()) {
				m_database.close();
			}

			m_database = QSqlDatabase();
			QSqlDatabase::removeDatabase(m_connectionName);
		}

	private:
		// current connection name
		QString m_connectionName;

		// current database type
		DatabaseType m_databaseType;

		// current connection
		QSqlDatabase m_database;

		void initializeMSSQLDB(const QVariantMap& connectionParameters)
		{
			m_connectionName = QStringLiteral("MSSQL-") + QUuid::createUuid().toString();
			m_database       = QSqlDatabase::addDatabase(QStringLiteral("QODBC"), m_connectionName);

			if (connectionParameters.empty()) {
				// clang-format off
				m_database.setDatabaseName(QStringLiteral("Driver=%1;Server=%2;Database=%3;Uid=%4;Pwd=%5;app=%6").arg(qApp->property("MSSQL_driver").toString(),
																													  qApp->property("MSSQL_host").toString(),
																													  qApp->property("MSSQL_database").toString(),
																													  qApp->property("MSSQL_user").toString(),
																													  qApp->property("MSSQL_password").toString(),
																													  qApp->property("MSSQL_application").toString()
																													  + QStringLiteral("@")
																													  + qApp->property("MSSQL_userID").toString()));
				// clang-format on
			} else {
				// clang-format off
				m_database.setDatabaseName(QStringLiteral("Driver=%1;Server=%2;Database=%3;Uid=%4;Pwd=%5").arg(connectionParameters["driver"].toString(),
																											   connectionParameters["host"].toString(),
																											   connectionParameters["database"].toString(),
																											   connectionParameters["user"].toString(),
																											   connectionParameters["password"].toString()));
				// clang-format on
			}

			if (!m_database.open()) {
				qWarning() << "Could not open connection:" << m_connectionName;
				qWarning() << m_database.lastError();
			}
		}

		void initializePSQLDB(const QVariantMap& connectionParameters)
		{
			m_connectionName = QStringLiteral("PSQL-") + QUuid::createUuid().toString();
			m_database       = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), m_connectionName);

			if (connectionParameters.empty()) {
				m_database.setHostName(qApp->property("QPSQL_host").toString());
				m_database.setDatabaseName(qApp->property("QPSQL_database").toString());
				m_database.setUserName(qApp->property("QPSQL_user").toString());
				m_database.setPassword(qApp->property("QPSQL_password").toString());
				m_database.setPort(qApp->property("QPSQL_port").toInt());
				m_database.setConnectOptions(QStringLiteral("connect_timeout=60"));
			} else {
				m_database.setHostName(connectionParameters["host"].toString());
				m_database.setDatabaseName(connectionParameters["database"].toString());
				m_database.setUserName(connectionParameters["user"].toString());
				m_database.setPassword(connectionParameters["password"].toString());
				m_database.setPort(connectionParameters["port"].toInt());
				m_database.setConnectOptions(QStringLiteral("connect_timeout=60"));
			}

			if (!m_database.open()) {
				qWarning() << "Could not open connection:" << m_connectionName;
				qWarning() << m_database.lastError();
			}
		}

		void initializeSQLITEDB(const QVariantMap& connectionParameters)
		{
			m_connectionName = QStringLiteral("SQLITE-") + QUuid::createUuid().toString();
			m_database       = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);

			if (connectionParameters.empty()) {
				// clang-format off
				m_database.setDatabaseName(qApp->property("QSQLITE_path").toString()
										   + QDir::separator()
										   + qApp->property("QSQLITE_database").toString());
				// clang-format on
			} else {
				// clang-format off
				m_database.setDatabaseName(connectionParameters["path"].toString()
								   + QDir::separator()
								   + connectionParameters["database"].toString());
				// clang-format on
			}

			if (!m_database.open()) {
				qWarning() << "Could not open connection:" << m_connectionName;
				qWarning() << m_database.lastError();
			}
		}
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLCONNECTOR_H
