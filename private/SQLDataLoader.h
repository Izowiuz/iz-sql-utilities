#ifndef IZSQLUTILITIES_SQLDATALOADER_H
#define IZSQLUTILITIES_SQLDATALOADER_H

//TODO: poprawić restartOperation()
//TODO: przywrócić funkcjonalność: m_countRaportingRequested = reportProgress

#include <future>

#include <QMap>
#include <QObject>
#include <QSqlError>
#include <QVariant>
#include <QVariantMap>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"

namespace IzSQLUtilities
{
	class SQLData;

	class SQLDataLoader : public QObject
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLDataLoader)

	public:
		// ctor
		explicit SQLDataLoader(QObject* parent = nullptr);

		// ctor with databaseType and databaseParameters
		explicit SQLDataLoader(IzSQLUtilities::DatabaseType databaseType,
							   const QVariantMap& databaseParameters,
							   QObject* parent = nullptr);

		// used to initialize data load
		void loadData(const QString& query,
					  bool asynchronousLoad                                        = true,
					  bool partialRefresh                                          = false,
					  bool reportProgress                                          = false,
					  bool emitOnAbort                                             = true,
					  const QPair<QString, QMap<int, QVariant>>& elementsToRefresh = QPair<QString, QMap<int, QVariant>>());

		// m_isLoadingData getter
		bool isLoadingData() const;

		// aborts data loading
		void abortOperation();

		// stops current operation without emiting sql data and starts a new on
		void restartOperation(const QString& newQuery);

		// m_databaseType getter / setter
		IzSQLUtilities::DatabaseType databaseType() const;
		void setDatabaseType(IzSQLUtilities::DatabaseType databaseType);

		// m_databaseParameters getter / setter
		QVariantMap databaseParameters() const;
		void setDatabaseParameters(const QVariantMap& databaseParameters);

	private:
		bool doWork(const QPair<QString, QMap<int, QVariant>>& elementsToRefresh = QPair<QString, QMap<int, QVariant>>());

		// loader's UUID
		QString m_UUID;

		// query used by the loader to load SQL data
		QString m_query;

		// true if count was requested
		// loader will be emmiting additional signals:
		// loadedCount(int) -> amount of currently loaded elements
		bool m_countRaportingRequested{ false };

		// controls loading type of the loader
		bool m_asynchronousLoad{ true };

		// controls how often data loader will be reporting about currenty loaded items
		int m_countReportingFrequency{ 10 };

		// true if performing partial refreshz*/
		bool m_partialRefresh{ false };

		// sends sqlError signal if SQL Error Interpreter is installed
		void sendSQLError(const QSqlError& error);

		// internal loading state
		bool m_isLoadingData{ false };

		// if true during abort loader will emit partially loaded data
		bool m_emitOnAbort{ true };

		// database type passed to SQLdbc constructor
		IzSQLUtilities::DatabaseType m_databaseType = IzSQLUtilities::DatabaseType::MSSQL;

		// database parameters passed to SQLdbc constructor
		QVariantMap m_databaseParameters;

		// true if abort was requested for this load opration
		std::atomic<bool> m_abortRequested{ false };

		// std future for the load operation
		std::future<bool> m_loadFuture;

		// true if next load operation was requested during existing one
		std::atomic<bool> m_restartOperation{ false };

	signals:
		// emited when SQLDataLoader finishes work
		void workCompleted(QSharedPointer<IzSQLUtilities::SQLData> data);

		// emited when query counting was requested, passes information about number of elements loaded
		void loadedCount(int count);
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLDATALOADER_H
