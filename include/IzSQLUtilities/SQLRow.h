#pragma once

#include <cstdio>
#include <vector>

#include <QVariant>

namespace IzSQLUtilities
{
    class SQLRow
    {
    public:
        // ctor
        SQLRow(std::size_t size);

        // dtor
        ~SQLRow() = default;

        // adds row to internal data
        void addColumnValue(const QVariant& value);

        // sets column to given value
        bool setColumnValue(int index, const QVariant& value);

        // returns column data for given index
        QVariant columnValue(int index) const;

    private:
        // size of sql row of data -> number of columns
        std::size_t m_size;

        // sql column values
        std::vector<QVariant> m_rowData;
    };
}   // namespace IzSQLUtilities
