#include "IzSQLUtilities/SQLFunctions.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMetaProperty>
#include <QSqlError>
#include <QSqlQuery>

#include "IzSQLUtilities/SQLErrorInterpreterA2.h"
#include "IzSQLUtilities/SQLdbc.h"

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"

IzSQLUtilities::SQLFunctions::SQLFunctions(QObject* parent)
	: QObject(parent)
{
}

bool IzSQLUtilities::SQLFunctions::callProcedure(const QString& functionName, const QString& sqlDefinition, const QVariantMap& parameters, bool emitStatusSignals)
{
	if (emitStatusSignals) {
		emit operationStarted(functionName);
	}
	if (sqlDefinition.isEmpty() || parameters.isEmpty()) {
		if (emitStatusSignals) {
			emit operationEnded(functionName);
		}
		qCritical() << "Sql definition or parameters list invaid.";
		return false;
	}
	SQLdbc db(functionName);
	if (db.initializeConnection()) {
		QSqlQuery query(db.getConnection());
		query.prepare(sqlDefinition);
		QMapIterator<QString, QVariant> i(parameters);
		while (i.hasNext()) {
			i.next();
			query.bindValue(QStringLiteral(":") + i.key(), i.value());
		}
		if (query.exec()) {
			qInfo() << "Function:" << functionName << "executed.";
			emit success(functionName);
			if (emitStatusSignals) {
				emit operationEnded(functionName);
			}
			return true;
		}
		SQLErrorInterpreterA2::instance()->sqlResponse(IzSQLUtilities::SQLResponseSeverity::SQL_RESPONSE_ERROR, query.lastError());
		if (emitStatusSignals) {
			emit operationEnded(functionName);
		}
		return false;
	}
	if (emitStatusSignals) {
		emit operationEnded(functionName);
	}
	SQLErrorInterpreterA2::instance()->sqlResponse(IzSQLUtilities::SQLResponseSeverity::SQL_RESPONSE_ERROR, db.lastError());
	return false;
}

bool IzSQLUtilities::SQLFunctions::callProcedure(const char* functionName, const char* sqlDefinition, const QVariantMap& parameters, bool emitStatusSignals)
{
	if (emitStatusSignals) {
		emit operationStarted(functionName);
	}
	if (parameters.isEmpty()) {
		if (emitStatusSignals) {
			emit operationEnded(functionName);
		}
		qCritical() << "Sql definition or parameters list invaid.";
		return false;
	}
	SQLdbc db(functionName);
	if (db.initializeConnection()) {
		QSqlQuery query(db.getConnection());
		query.prepare(sqlDefinition);
		QMapIterator<QString, QVariant> i(parameters);
		while (i.hasNext()) {
			i.next();
			query.bindValue(QStringLiteral(":") + i.key(), i.value());
		}
		if (query.exec()) {
			qInfo() << "Function:" << functionName << "executed.";
			emit success(functionName);
			if (emitStatusSignals) {
				emit operationEnded(functionName);
			}
			return true;
		}
		SQLErrorInterpreterA2::instance()->sqlResponse(IzSQLUtilities::SQLResponseSeverity::SQL_RESPONSE_ERROR, query.lastError());
		if (emitStatusSignals) {
			emit operationEnded(functionName);
		}
		return false;
	}
	if (emitStatusSignals) {
		emit operationEnded(functionName);
	}
	SQLErrorInterpreterA2::instance()->sqlResponse(IzSQLUtilities::SQLResponseSeverity::SQL_RESPONSE_ERROR, db.lastError());
	return false;
}

bool IzSQLUtilities::SQLFunctions::objectNameAvailable(const QString& table, const QString& column, const QString& object)
{
	QString tTable  = sanitize(table);
	QString tColumn = sanitize(column);
	QString tObject = sanitize(object);
	SQLdbc db(QStringLiteral("check-object-name-avability"));
	if (db.initializeConnection()) {
		QSqlQuery checkName(db.getConnection());
		checkName.prepare(QStringLiteral("SELECT COUNT(id) FROM ") + tTable + QStringLiteral(" WHERE ") + tColumn + QStringLiteral(" = '") + tObject + QStringLiteral("'"));
		if (checkName.exec()) {
			checkName.first();
			return (checkName.value(0).toInt() == 0);
		}
		qCritical() << checkName.lastError().text();
		SQLErrorInterpreterA2::instance()->sqlResponse(IzSQLUtilities::SQLResponseSeverity::SQL_RESPONSE_ERROR, checkName.lastError());
		return false;
	}
	SQLErrorInterpreterA2::instance()->sqlResponse(IzSQLUtilities::SQLResponseSeverity::SQL_RESPONSE_ERROR, db.lastError());
	return false;
}

bool IzSQLUtilities::SQLFunctions::objectNameAvailableWithConstrain(const QString& table, const QString& column, const QString& object, const QString& type, int typeID)
{
	QString tTable  = sanitize(table);
	QString tColumn = sanitize(column);
	QString tObject = sanitize(object);
	QString tType   = sanitize(type);
	SQLdbc db(QStringLiteral("check-object-name-avability-with-constrain"));
	if (db.initializeConnection()) {
		QSqlQuery checkName(db.getConnection());
		checkName.prepare(QStringLiteral("SELECT COUNT(id) FROM ") + tTable + QStringLiteral(" WHERE ") + tColumn + QStringLiteral(" = '") + tObject + QStringLiteral("' AND ") + tType + QStringLiteral(" = ") + QString::number(typeID));
		if (checkName.exec()) {
			checkName.first();
			return (checkName.value(0).toInt() == 0);
		}
		qCritical() << checkName.lastError().text();
		SQLErrorInterpreterA2::instance()->sqlResponse(IzSQLUtilities::SQLResponseSeverity::SQL_RESPONSE_ERROR, checkName.lastError());
		return false;
	}
	SQLErrorInterpreterA2::instance()->sqlResponse(IzSQLUtilities::SQLResponseSeverity::SQL_RESPONSE_ERROR, db.lastError());
	return false;
}

const QString IzSQLUtilities::SQLFunctions::sanitize(const QString& parameter) const
{
	QString out = parameter;
	out.remove(QStringLiteral("DELETE"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("UPDATE"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("SELECT"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("FROM"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("LIKE"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("NOT LIKE"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("TRUNCATE"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("DROP"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("CREATE"), Qt::CaseInsensitive);
	out.remove(QStringLiteral("'"), Qt::CaseInsensitive);
	out.remove(QStringLiteral(";"), Qt::CaseInsensitive);
	out.remove(QStringLiteral(";"), Qt::CaseInsensitive);
	return out;
}
