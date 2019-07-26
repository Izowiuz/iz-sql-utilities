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

	enum class DatabaseType : uint8_t {
		MSSQL = 0,
		SQLITE,
		PSQL
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_ENUMS_H
