#ifndef IZSQLUTILITIES_SQLCONNECTOR_H
#define IZSQLUTILITIES_SQLCONNECTOR_H

#include <type_traits>

#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QThreadStorage>
#include <QUrl>
#include <QUuid>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"
#include "IzSQLUtilities_Global.h"

namespace IzSQLUtilities
{
	template <IzSQLUtilities::DatabaseType DBType, typename = std::enable_if_t<std::is_same<decltype(DBType), IzSQLUtilities::DatabaseType>::value>>
	class SqlConnector
	{
		friend class ConnectionPool;

	public:
		// connection parameters
		template <IzSQLUtilities::DatabaseType U>
		struct ConnectionParameters;

		// ctors
		template <IzSQLUtilities::DatabaseType U>
		SqlConnector() = delete;

		// disallow copy
		template <IzSQLUtilities::DatabaseType U>
		SqlConnector(SqlConnector& other) = delete;

		template <IzSQLUtilities::DatabaseType U>
		SqlConnector(const SqlConnector& other) = delete;

		// disallow move
		template <IzSQLUtilities::DatabaseType U>
		SqlConnector(SqlConnector&& other) = delete;

		explicit SqlConnector(const ConnectionParameters<DBType>& connectionParameters = ConnectionParameters<DBType>());

		// dtor - closses non-pooled connection on object destruction
		~SqlConnector()
		{
			if (!m_isPooled) {
				closeConnection();
			}
		}

		// returns created connection type
		IzSQLUtilities::DatabaseType connectionType() const
		{
			return m_connectionType;
		}

		// returns created database connection
		QSqlDatabase getDatabase() const
		{
			return QSqlDatabase::database(m_connectionName, false);
		}

		// returns last error generated for this connection
		const QSqlError getLastError() const
		{
			return QSqlDatabase::database(m_connectionName, false).lastError();
		}

		// returns generated connection name
		const QString getConnectionName() const
		{
			return m_connectionName;
		}

		// m_isPooled getter
		bool isPooled() const
		{
			return m_isPooled;
		}

		// closes generated connection
		void closeConnection()
		{
			{
				auto db = getDatabase();
				if (db.isOpen()) {
					db.close();
				} else {
					qWarning() << "Attempting to close connection:"
							   << m_connectionName
							   << "but it is not opened.";
				}
			}
			QSqlDatabase::removeDatabase(m_connectionName);
		}

	private:
		// current connection name
		QString m_connectionName;

		// current connection type
		IzSQLUtilities::DatabaseType m_connectionType;

		// true if connection is pooled
		// if it is - it will not be closed on SqlConnector destruction
		bool m_isPooled{ false };

		// initializes this Connector as being pooled
		void makePooled()
		{
			m_isPooled = true;
		}
	};

	template <>
	template <>
	struct SqlConnector<IzSQLUtilities::DatabaseType::SQLITE>::ConnectionParameters<IzSQLUtilities::DatabaseType::SQLITE> {
		// database name
		QString database;

		// database path
		QUrl path;
	};

	template <>
	template <>
	struct SqlConnector<IzSQLUtilities::DatabaseType::PSQL>::ConnectionParameters<IzSQLUtilities::DatabaseType::PSQL> {
		// target host
		QString host;

		// name of the database
		QString database;

		// database user
		QString user;

		// user's password
		QString password;

		// database listening port
		int port{ -1 };
	};

	template <>
	template <>
	struct SqlConnector<IzSQLUtilities::DatabaseType::MSSQL>::ConnectionParameters<IzSQLUtilities::DatabaseType::MSSQL> {
		// target host
		QString host;

		// name of the database
		QString database;

		// database user
		QString user;

		// user's password
		QString password;

		// driver to use
		QString driver;
	};

	template <>
	inline SqlConnector<IzSQLUtilities::DatabaseType::SQLITE>::SqlConnector(const ConnectionParameters<IzSQLUtilities::DatabaseType::SQLITE>& connectionParameters)
		: m_connectionName(QUuid::createUuid().toString())
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
		db.setDatabaseName(connectionParameters.path.toString() + QDir::separator() + connectionParameters.database);
		if (!db.open()) {
			qWarning() << "Could not open connection:" << m_connectionName;
			qWarning() << db.lastError();
		}
	}

	template <>
	inline SqlConnector<IzSQLUtilities::DatabaseType::PSQL>::SqlConnector(const ConnectionParameters<IzSQLUtilities::DatabaseType::PSQL>& connectionParameters)
		: m_connectionName(QUuid::createUuid().toString())
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), m_connectionName);
		db.setHostName(connectionParameters.host);
		db.setDatabaseName(connectionParameters.database);
		db.setUserName(connectionParameters.user);
		db.setPassword(connectionParameters.password);
		db.setPort(connectionParameters.port);
		db.setConnectOptions(QStringLiteral("connect_timeout=60"));
		if (!db.open()) {
			qWarning() << "Could not open connection:" << m_connectionName;
			qWarning() << db.lastError();
		}
	}

	template <>
	inline SqlConnector<IzSQLUtilities::DatabaseType::MSSQL>::SqlConnector(const ConnectionParameters<IzSQLUtilities::DatabaseType::MSSQL>& connectionParameters)
		: m_connectionName(QUuid::createUuid().toString())
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QODBC"), m_connectionName);
		db.setDatabaseName(QStringLiteral("Driver=%1;Server=%2;Database=%3;Uid=%4;Pwd=%5").arg(connectionParameters.driver,
																							   connectionParameters.host,
																							   connectionParameters.database,
																							   connectionParameters.user,
																							   connectionParameters.password));
		if (!db.open()) {
			qWarning() << "Could not open connection:" << m_connectionName;
			qWarning() << db.lastError();
		}
	}

	class IZSQLUTILITIESSHARED_EXPORT ConnectionPool
	{
		// disallow copy
		Q_DISABLE_COPY(ConnectionPool)
	public:
		// ctor
		ConnectionPool() = default;

		// disallow move
		ConnectionPool(ConnectionPool&& other) = delete;

		// dtor
		virtual ~ConnectionPool() final = default;

		// instance getter
		static ConnectionPool* instance();

		// connection pool getters
		QThreadStorage<QSqlDatabase>& pool()
		{
			return m_connectionPool;
		}

		const QThreadStorage<QSqlDatabase>& connectionPool() const
		{
			return m_connectionPool;
		}

		template <IzSQLUtilities::DatabaseType DBType, typename... Args>
		QSqlDatabase database(Args&&... connectionParameters)
		{
			if (ConnectionPool::instance()->pool().hasLocalData()) {
				return ConnectionPool::instance()->pool().localData();
			}
			IzSQLUtilities::SqlConnector<DBType> c(std::forward<Args>(connectionParameters)...);
			c.makePooled();
			ConnectionPool::instance()->pool().setLocalData(c.getDatabase());
			return ConnectionPool::instance()->pool().localData();
		}

	private:
		// thread local connection pool
		QThreadStorage<QSqlDatabase> m_connectionPool;
	};

	using MSSQLParameters  = IzSQLUtilities::SqlConnector<IzSQLUtilities::DatabaseType::MSSQL>::ConnectionParameters<IzSQLUtilities::DatabaseType::MSSQL>;
	using PSQLParameters   = IzSQLUtilities::SqlConnector<IzSQLUtilities::DatabaseType::PSQL>::ConnectionParameters<IzSQLUtilities::DatabaseType::PSQL>;
	using SQLITEParameters = IzSQLUtilities::SqlConnector<IzSQLUtilities::DatabaseType::SQLITE>::ConnectionParameters<IzSQLUtilities::DatabaseType::SQLITE>;
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLCONNECTOR_H
