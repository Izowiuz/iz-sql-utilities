#pragma once

#include <QEvent>

namespace IzSQLUtilities
{
    enum class DatabaseType : uint8_t {
        MSSQL = 0,
        SQLITE,
        PSQL
    };
}   // namespace IzSQLUtilities
