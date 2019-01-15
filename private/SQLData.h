#ifndef IZSQLUTILITIES_SQLDATA_H
#define IZSQLUTILITIES_SQLDATA_H

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QSharedPointer>
#include <QSqlError>
#include <QVariantMap>
#include <QVector>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"

namespace IzSQLUtilities
{
	class SQLDataContainer;

	class IZSQLUTILITIESSHARED_EXPORT SQLData
	{
	public:
		// ctor
		explicit SQLData();

		// sql column namses setter / getter
		QStringList getSQLColumnNames() const;
		void setSQLColumnNames(const QStringList& columnNames);

		// sql data getter
		QVector<QSharedPointer<SQLDataContainer>> sqlData() const;

		// adds one element to the data list
		void addRow(QSharedPointer<SQLDataContainer> row);

		// m_sqlError getter
		QSqlError sqlError() const;

		// m_loadStatus getter
		ModelLoadStatus loadStatus() const;

		// m_refreshedElements getter / setter
		bool getPartialRefresh() const;
		void setPartialRefresh(bool partialRefresh);

		// m_refreshedElements getter / setter
		QPair<QString, QMap<int, QVariant>> getRefreshedElements() const;
		void setRefreshedElements(const QPair<QString, QMap<int, QVariant>>& indexValueMap);

		// m_removedElements getter / setter
		QVector<int> getRemovedElements() const;
		void setRemovedElements(const QVector<int>& removedElements);

		// m_loadStatus setter
		void setLoadStatus(const ModelLoadStatus& loadStatus);

		// clears internal data - without headers
		void clearData();

		// m_sqlError setter
		void setSqlError(const QSqlError& sqlError);

		// column -> index relations setter / getter
		QHash<QString, int> getColumnIndexMap() const;
		void setColumnIndexMap(const QHash<QString, int>& columnIndexMap);

		// index relations setter / getter
		QHash<int, QString> getIndexColumnMap() const;
		void setIndexColumnMap(const QHash<int, QString>& indexColumnMap);

	private:
		// actual sql data from db
		QVector<QSharedPointer<SQLDataContainer>> m_sqlData;

		// map of column -> index relations
		QHash<QString, int> m_columnIndexMap;

		// map of index -> column relations
		QHash<int, QString> m_indexColumnMap;

		// sql column names
		QStringList m_sqlColumnNames;

		// sql error returned when load operiation fails
		QSqlError m_sqlError;

		// load operation ststus
		ModelLoadStatus m_loadStatus{ ModelLoadStatus::NOT_INITIALIZED };

		// true if opertion is partial refresh and false otherwise
		bool m_partialRefresh{ false };

		// used during partial refresh
		// contains index of the element in the model and the identity value from the database
		QPair<QString, QMap<int, QVariant>> m_refreshedElements;

		// used during partial refresh
		// contains list of the model indexes that were removed in the process of partial refresh
		QVector<int> m_removedElements;
	};
}   // namespace IzSQLUtilities

Q_DECLARE_METATYPE(QSharedPointer<IzSQLUtilities::SQLData>)

#endif   // IZSQLUTILITIES_SQLDATA_H
