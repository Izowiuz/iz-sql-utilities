#include "IzSQLUtilities/SQLDataContainer.h"

#include <QDebug>

IzSQLUtilities::SQLDataContainer::SQLDataContainer(unsigned int containerSize)
	: m_containerSize(containerSize)
{
	m_containerData.reserve(m_containerSize);
}

IzSQLUtilities::SQLDataContainer::SQLDataContainer(unsigned int containerSize, const QVariantMap& fieldValues, const QStringList& sqlColumns)
	: m_isAdded(true)
	, m_containerSize(containerSize)
{
	m_containerData.reserve(m_containerSize);
	setIsInitializing(true);
	for (const auto& sqlColumn : qAsConst(sqlColumns)) {
		if (fieldValues.contains(sqlColumn)) {
			addField(fieldValues.value(sqlColumn));
		} else {
			qCritical() << "SQL column:" << sqlColumn << "not found in element provided field definitions. Container is invalid.";
			m_isValid = false;
		}
	}
	setIsInitializing(false);
}

bool IzSQLUtilities::SQLDataContainer::isAdded() const
{
	return m_isAdded;
}

bool IzSQLUtilities::SQLDataContainer::toBeRemoved() const
{
	return m_toBeRemoved;
}

bool IzSQLUtilities::SQLDataContainer::isDirty() const
{
	if (m_isAdded || m_toBeRemoved) {
		return true;
	}
	return !m_changedFields.empty();
}

bool IzSQLUtilities::SQLDataContainer::isValid() const
{
	return m_isValid;
}

bool IzSQLUtilities::SQLDataContainer::isInitializing() const
{
	return m_isInitializing;
}

void IzSQLUtilities::SQLDataContainer::setToBeRemoved(bool toBeRemoved)
{
	m_toBeRemoved = toBeRemoved;
}

void IzSQLUtilities::SQLDataContainer::setIsAdded(bool isAdded)
{
	m_isAdded = isAdded;
}

void IzSQLUtilities::SQLDataContainer::setIsInitializing(bool isInitializing)
{
	m_isInitializing = isInitializing;
}

unsigned int IzSQLUtilities::SQLDataContainer::size() const
{
	return m_containerSize;
}

QVariant IzSQLUtilities::SQLDataContainer::fieldValue(int fieldIndex) const
{
	if (static_cast<unsigned int>(fieldIndex) > m_containerSize) {
		qCritical() << "Field index:" << fieldIndex << "is out of range.";
		return QVariant();
	}
	return m_containerData[static_cast<unsigned int>(fieldIndex)];
}

std::unordered_map<int, QVariant> IzSQLUtilities::SQLDataContainer::getChangedFields() const
{
	return m_changedFields;
}

bool IzSQLUtilities::SQLDataContainer::setFieldValue(int fieldIndex, const QVariant& fieldValue)
{
	if (static_cast<unsigned int>(fieldIndex) > m_containerSize) {
		qCritical() << "Field index:" << fieldIndex << "is out of range.";
		return false;
	}
	if (m_containerData.at(static_cast<unsigned int>(fieldIndex)) == fieldValue) {
		return true;
	}
	const auto changedIdx = m_changedFields.find(fieldIndex);
	if (changedIdx == m_changedFields.cend()) {
		m_changedFields.emplace(fieldIndex, m_containerData.at(static_cast<unsigned int>(fieldIndex)));
		m_containerData.at(static_cast<unsigned int>(fieldIndex)) = fieldValue;
		return true;
	}
	if (changedIdx->second == fieldValue) {
		m_containerData.at(static_cast<unsigned int>(fieldIndex)) = changedIdx->second;
		m_changedFields.erase(changedIdx);
	} else {
		m_containerData.at(static_cast<unsigned int>(fieldIndex)) = fieldValue;
	}
	return true;
}

void IzSQLUtilities::SQLDataContainer::addField(const QVariant& fieldValue)
{
	if (!isInitializing()) {
		qCritical() << "addField() called post initialization.";
		return;
	}
	m_containerData.emplace_back(fieldValue);
}

bool IzSQLUtilities::SQLDataContainer::fieldIsDirty(int fieldIndex)
{
	if (static_cast<unsigned int>(fieldIndex) > m_containerSize) {
		qCritical() << "Field index:" << fieldIndex << "is out of range.";
		return false;
	}
	return m_changedFields.find(fieldIndex) != m_changedFields.cend();
}

bool IzSQLUtilities::SQLDataContainer::cleanField(int fieldIndex)
{
	if (static_cast<unsigned int>(fieldIndex) > m_containerSize) {
		qCritical() << "Field index:" << fieldIndex << "is out of range.";
		return false;
	}
	const auto changedIdx = m_changedFields.find(fieldIndex);
	if (changedIdx == m_changedFields.cend()) {
		qCritical() << "Field index:" << fieldIndex << "is already clean.";
		return false;
	}
	m_containerData.at(static_cast<unsigned int>(fieldIndex)) = changedIdx->second;
	m_changedFields.erase(changedIdx);
	return true;
}

bool IzSQLUtilities::SQLDataContainer::cleanContainer()
{
	if (!isDirty()) {
		qInfo() << "Container is already clean.";
		return false;
	}
	for (const auto& element : m_changedFields) {
		m_containerData.at(static_cast<unsigned int>(element.first)) = element.second;
	}
	m_changedFields.clear();
	m_toBeRemoved = false;
	return true;
}

std::vector<QVariant> IzSQLUtilities::SQLDataContainer::containerData() const
{
	return m_containerData;
}
