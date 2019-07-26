#ifndef IZSQLUTILITIES_ENUMS_H
#define IZSQLUTILITIES_ENUMS_H

#include <QEvent>

namespace IzSQLUtilities
{
	enum class DatabaseType : uint8_t {
		MSSQL = 0,
		SQLITE,
		PSQL
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_ENUMS_H
