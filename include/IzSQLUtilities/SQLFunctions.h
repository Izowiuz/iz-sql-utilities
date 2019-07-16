#ifndef IZSQLUTILITIES_SQLFUNCTIONS_H
#define IZSQLUTILITIES_SQLFUNCTIONS_H

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"
#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QObject>
#include <QSharedPointer>
#include <QSqlError>
#include <QVariantMap>

namespace IzSQLUtilities
{
	class IZSQLUTILITIESSHARED_EXPORT SQLFunctions : public QObject
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLFunctions)

	public:
		// ctor
		explicit SQLFunctions(QObject* parent = nullptr);

		// dtor
		~SQLFunctions() = default;

		// directly calls sql procedure
		Q_INVOKABLE bool callProcedure(const QString& functionName, const QString& sqlDefinition, const QVariantMap& parameters, bool emitStatusSignals = true);
		Q_INVOKABLE bool callProcedure(const char* functionName, const char* sqlDefinition, const QVariantMap& parameters, bool emitStatusSignals = true);

		// static, state less variant of the callProcedure function
		static bool callProcedureStatic(const char* functionName, const char* sqlDefinition, const QVariantMap& parameters, DatabaseType databaseType = DatabaseType::MSSQL, const QVariantMap& connectionParameters = {});

		// checks for SQL object avability in given table and column
		Q_INVOKABLE bool objectNameAvailable(const QString& table, const QString& column, const QString& object);

		// checks for SQL object avability in given table and column
		Q_INVOKABLE bool objectNameAvailableWithConstrain(const QString& table, const QString& column, const QString& object, const QString& type, int typeID);

		// m_connectionParameters setter / getter
		QVariantMap connectionParameters() const;
		void setConnectionParameters(const QVariantMap& connectionParameters);

		// m_databaseType setter / getter
		IzSQLUtilities::DatabaseType databaseType() const;
		void setDatabaseType(const IzSQLUtilities::DatabaseType& databaseType);

		// sql database type
		IzSQLUtilities::DatabaseType m_databaseType{ IzSQLUtilities::DatabaseType::MSSQL };

		// sql connection parameters
		QVariantMap m_connectionParameters;

	private:
		// tries to sanitize sql parameter for dynamic queries
		QString sanitize(const QString& parameter) const;

	signals:
		// emited when procedure successfully finished
		void success(QString operation);

		// emited when function have started
		void operationStarted(QString operation);

		// emited when function have ended
		void operationEnded(QString operation);
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLFUNCTIONS_H
