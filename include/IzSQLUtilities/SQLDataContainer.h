#ifndef IZSQLUTILITIES_SQLDATACONTAINER_H
#define IZSQLUTILITIES_SQLDATACONTAINER_H

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <unordered_map>
#include <vector>

#include <QVariant>

namespace IzSQLUtilities
{
	class IZSQLUTILITIESSHARED_EXPORT SQLDataContainer
	{
	public:
		// default ctor
		explicit SQLDataContainer(unsigned int containerSize);

		// ctor with predefined datafield values
		// this ctor always sets isAdded to true on created element
		explicit SQLDataContainer(unsigned int containerSize, const QVariantMap& fieldValues, const QStringList& sqlColumns);

		// dtor
		~SQLDataContainer() = default;

		// container states getters
		bool isAdded() const;
		bool toBeRemoved() const;
		bool isDirty() const;
		bool isValid() const;
		bool isInitializing() const;

		// container states setters
		void setToBeRemoved(bool toBeRemoved);
		void setIsAdded(bool isAdded);
		void setIsInitializing(bool isInitializing);

		// returns size - number of fields - for this container
		unsigned int size() const;

		// returns datafield value for the given datafield index or invalid QVariant() if the index is not in the valid range
		QVariant fieldValue(int fieldIndex) const;

		// sets field value
		// post-dataload function does not allow to set additional dynamic properties on this object
		// returns true on success and false otherwise
		bool setFieldValue(int fieldIndex, const QVariant& fieldValue);

		// adds new datafield to the container data
		void addField(const QVariant& fieldValue);

		// m_changedFields getter
		std::unordered_map<int, QVariant> getChangedFields() const;

		// returns true if field was modified and false otherwise
		bool fieldIsDirty(int fieldIndex);

		// cleans field, returns true on success and false otherwise
		bool cleanField(int fieldIndex);

		// cleans container: m_changedFields is cleared and all initial values restored
		// returns true on success and false otherwise
		bool cleanContainer();

		// returns container data
		// WARNING: zwraca kopię danych
		std::vector<QVariant> containerData() const;

		// returns iterators for m_containerData vector
		auto begin() { return m_containerData.begin(); };
		auto end() { return m_containerData.end(); };

		// returns const iterators for m_containerData vector
		const auto cbegin() const { return m_containerData.cbegin(); };
		const auto cend() const { return m_containerData.cend(); };

	private:
		// container data
		std::vector<QVariant> m_containerData;

		// stores indexes of the changed fields with their initial values
		std::unordered_map<int, QVariant> m_changedFields;

		// container states
		bool m_isAdded{ false };
		bool m_toBeRemoved{ false };
		bool m_isValid{ true };
		bool m_isInitializing{ false };

		// internal size of the container, equal to the number of columns from sql query
		unsigned int m_containerSize;
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLDATACONTAINER_H
