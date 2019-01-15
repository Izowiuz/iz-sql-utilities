#ifndef IZSQLUTILITIES_LOADEDSQLDATA_H
#define IZSQLUTILITIES_LOADEDSQLDATA_H

#include <memory>
#include <vector>

#include <QHash>

#include "IzSQLUtilities/SQLDataContainer.h"

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
		std::vector<std::unique_ptr<SQLDataContainer>>& sqlData();

		// m_columnIndexMap getter / setter
		QHash<QString, int> columnIndexMap() const;
		void setColumnIndexMap(const QHash<QString, int>& columnIndexMap);

		// m_indexColumnMap getter / setter
		QHash<int, QString> indexColumnMap() const;
		void setIndexColumnMap(const QHash<int, QString>& indexColumnMap);

		// m_sqlData getter - moves row into internal data structure
		void addRow(std::unique_ptr<SQLDataContainer> row);

	private:
		// raw sql data from db
		std::vector<std::unique_ptr<SQLDataContainer>> m_sqlData;

		// map of column -> index relations
		QHash<QString, int> m_columnIndexMap;

		// map of index -> column relations
		QHash<int, QString> m_indexColumnMap;
	};

}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_LOADEDSQLDATA_H
