#ifndef IZSQLUTILITIES_LOADEDSQLDATA_H
#define IZSQLUTILITIES_LOADEDSQLDATA_H

#include <memory>
#include <vector>

#include <QHash>
#include <QVariant>

#include "IzSQLUtilities/SQLRow.h"

namespace IzSQLUtilities
{
	class LoadedSQLData
	{
	public:
		// ctor
		LoadedSQLData() = default;

		// dtor
		~LoadedSQLData() = default;

		// m_sqlData getter / setter
		std::vector<std::unique_ptr<SQLRow>>& sqlData();

		// m_columnIndexMap getter / setter
		QHash<QString, int> columnIndexMap() const;
		void setColumnIndexMap(const QHash<QString, int>& columnIndexMap);

		// m_indexColumnMap getter / setter
		QMap<int, QString> indexColumnMap() const;
		void setIndexColumnMap(const QMap<int, QString>& indexColumnMap);

		// m_sqlData getter - moves row into internal data structure
		void addRow(std::unique_ptr<SQLRow> row);

		// m_sqlDataTypes getter / setter
		std::vector<QMetaType::Type>& sqlDataTypes();
		void setSqlDataTypes(const std::vector<QMetaType::Type>& sqlDataTypes);

	private:
		// raw sql data from db
		std::vector<std::unique_ptr<SQLRow>> m_sqlData;

		// sql column data types
		std::vector<QMetaType::Type> m_sqlDataTypes;

		// map of column -> index relations
		QHash<QString, int> m_columnIndexMap;

		// map of index -> column relations
		QMap<int, QString> m_indexColumnMap;
	};

}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_LOADEDSQLDATA_H
