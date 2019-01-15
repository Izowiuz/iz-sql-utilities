#ifndef IZSQLUTILITIES_ENUMS_H
#define IZSQLUTILITIES_ENUMS_H

#include <QEvent>

namespace IzSQLUtilities
{
	enum class ModelLoadStatus {
		NOT_INITIALIZED,
		LOADING,
		LOADED,
		ABORTED,
		QUERY_EMPTY,
		QUERY_ERROR,
		SQL_ERROR
	};

	enum class SQLResponseSeverity : int {
		SQL_RESPONSE_INFO  = QEvent::User + 1,
		SQL_RESPONSE_ERROR = QEvent::User + 2
	};

	enum class DatabaseType : uint8_t {
		MSSQL = 0,
		SQLITE
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_ENUMS_H
