#ifndef IZSQLUTILITIES_SQLFUNCTIONS_H
#define IZSQLUTILITIES_SQLFUNCTIONS_H

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

		// checks for SQL object avability in given table and column*/
		Q_INVOKABLE bool objectNameAvailable(const QString& table, const QString& column, const QString& object);

		// checks for SQL object avability in given table and column*/
		Q_INVOKABLE bool objectNameAvailableWithConstrain(const QString& table, const QString& column, const QString& object, const QString& type, int typeID);

	private:
		// tries to sanitize sql parameter for dynamic queries
		const QString sanitize(const QString& parameter) const;

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
