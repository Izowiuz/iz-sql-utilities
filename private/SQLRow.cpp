#include "IzSQLUtilities/SQLRow.h"

#include <QDebug>

IzSQLUtilities::SQLRow::SQLRow(std::size_t size)
	: m_size(size)
{
	m_rowData.reserve(size);
}

void IzSQLUtilities::SQLRow::addColumnValue(const QVariant& value)
{
	if (m_rowData.size() + 1 > m_size) {
		qCritical() << "Cannot add new value to this SQL data row. Row is already at maximum capacity.";
		return;
	}
	m_rowData.push_back(value);
}

bool IzSQLUtilities::SQLRow::setColumnValue(int index, const QVariant &value)
{
	if (index < 0 || static_cast<std::size_t>(index) > m_size) {
		qCritical() << "Got invalid index for this data row:" << index;
		return false;
	}
	m_rowData[index] = value;
	return true;
}

QVariant IzSQLUtilities::SQLRow::columnValue(int index) const
{
	if (index < 0 || static_cast<std::size_t>(index) > m_size) {
		qCritical() << "Got invalid index for this data row:" << index;
	}
	return m_rowData[index];
}
