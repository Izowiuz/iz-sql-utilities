#ifndef IZSQLUTILITIES_SQLTABLEMODEL_H
#define IZSQLUTILITIES_SQLTABLEMODEL_H

#include <atomic>
#include <memory>
#include <vector>

#include <QAbstractItemModel>
#include <QFutureWatcher>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"
#include "IzSQLUtilities/IzSQLUtilities_Global.h"
#include "SQLDataContainer.h"

// TODO: przepisać normalniej funkcję validującą sql query i jego parametry
// TODO: może jakaś abstrakcyjny interfejs dla modelu danych?
// TODO: zastanowić się nad nową implementacją SQLDataContainer
// TODO: implementacja funkcjonalności abortLoading()
// TODO: implementacja funkcjonalności częściowego refresh'a
// TODO: sterowanie częstotliwością wysyłania sygnału rowsLoaded(int)
// TODO: typy kolumn w LoadedSQLData

namespace IzSQLUtilities
{
	class LoadedSQLData;

	class IZSQLUTILITIESSHARED_EXPORT SQLTableModel : public QAbstractItemModel
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLTableModel)

		// sql query
		Q_PROPERTY(QString sqlQuery READ sqlQuery WRITE setSqlQuery NOTIFY sqlQueryChanged FINAL)

		// sql query parameters
		Q_PROPERTY(QVariantMap sqlQueryParameters READ sqlQueryParameters WRITE setSqlQueryParameters NOTIFY sqlQueryParametersChanged FINAL)

		// true if query is valid
		Q_PROPERTY(bool queryIsValid READ queryIsValid NOTIFY queryIsValidChanged)

		// true if model is currently refreshing
		Q_PROPERTY(bool isRefreshing READ isRefreshing NOTIFY isRefreshingChanged FINAL)

		// count of the elements in the model
		Q_PROPERTY(int count READ rowCount NOTIFY isRefreshingChanged FINAL)

		// types of data refresh
	public:
		enum class DataRefreshType : uint8_t {
			Full = 0,
			Partial
		};
		Q_ENUMS(DataRefreshType)

		enum class DataRefreshResult : uint8_t {
			Refreshed = 0,
			DatabaseError,
			QueryError
		};
		Q_ENUMS(DataRefreshType)

		enum class SQLTableModelRoles : int {
			// defined for consistency in implementation of data() function
			DisplayData = Qt::DisplayRole,
			// true if element was added by post load means
			IsAdded = Qt::UserRole,
			// true if element was marked as being to be removed from external data set
			ToBeRemoved = Qt::UserRole + 1,
			// true if, post load or post add, any of the element's fields were changed
			IsDirty = Qt::UserRole + 2,
			// true if newly constructed element has fields incompatibile with data set's
			IsValid = Qt::UserRole + 3,
			// true if index is selected
			IsSelected = Qt::UserRole + 4
		};

		using LoadedData = std::tuple<SQLTableModel::DataRefreshResult, SQLTableModel::DataRefreshType, std::shared_ptr<LoadedSQLData>>;

		// ctors
		explicit SQLTableModel(QObject* parent = nullptr);

		// dtor
		~SQLTableModel() = default;

		// QAbstractItemModel interface start

		// header
		QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
		bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) override;

		// basic functionality
		QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
		QModelIndex parent(const QModelIndex& index) const override;
		int rowCount(const QModelIndex& parent = QModelIndex()) const override;
		int columnCount(const QModelIndex& parent = QModelIndex()) const override;

		// fetch data dynamically
		bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
		bool canFetchMore(const QModelIndex& parent) const override;
		void fetchMore(const QModelIndex& parent) override;

		// get data
		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

		// set data
		bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;

		// add data
		bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
		bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

		// remove data
		bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
		bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

		// QML roles
		QHash<int, QByteArray> roleNames() const override;

		// QAbstractItemModel interface stop

		// custom model interface start

		// returns true if given index is in valid range for this model instance
		inline bool indexIsValid(int index) const
		{
			return index > static_cast<int>(m_data.size()) && index > m_indexColumnMap.size();
		};

		// returns column name for given column index
		QString columnNameFromIndex(int index) const;

		// returns index for given column name
		int indexFromColumnName(const QString& column) const;

		// m_sqlQuery getter / setter
		QString sqlQuery() const;
		void setSqlQuery(const QString& sqlQuery);

		// m_sqlQueryParameters getter / setter
		QVariantMap sqlQueryParameters() const;
		void setSqlQueryParameters(const QVariantMap& sqlQueryParameters);

		// used to refresh data, emits refreshStarted signal, sets sqlQuery and sqlQueryParameters
		Q_INVOKABLE void refreshData(const QString& sqlQuery, const QVariantMap& sqlParameters, const QList<int>& rows = {});

		// used to refresh data, emits refreshStarted signal, uses set earlier sqlQuery and sqlQueryParameters members
		Q_INVOKABLE void refreshData();

		// m_isRefreshing getter
		bool isRefreshing() const;

		// custom model interface end

		// returns iterators for m_data vector
		auto begin()
		{
			return m_data.begin();
		};
		auto end()
		{
			return m_data.end();
		};

		// returns const iterators for m_data vector
		const auto cbegin() const
		{
			return m_data.cbegin();
		};
		const auto cend() const
		{
			return m_data.cend();
		};

		// WARNING: absolutely no boundary checks
		SQLDataContainer& at(int index)
		{
			return *m_data[index];
		}

		// m_queryIsValid getter
		bool queryIsValid() const;

		// m_columnNameColumnAliasMap getter / setter
		QVariantMap columnNameColumnAliasMap() const;
		void setColumnNameColumnAliasMap(const QVariantMap& columnNameColumnAliasMap);

	private:
		// internal data of the model
		std::vector<std::unique_ptr<SQLDataContainer>> m_data;

		// SQL column names -> indexes relations
		// contains valid values only post data load
		QHash<QString, int> m_columnIndexMap;

		// indexes -> SQL column names relations
		// contains valid values only post data load
		QHash<int, QString> m_indexColumnMap;

		// column names -> column aliases relations
		QVariantMap m_columnNameColumnAliasMap;

		// cached role names names for use under QML views
		mutable QHash<int, QByteArray> m_cachedRoleNames;

		// raw sql query
		QString m_sqlQuery;

		// sql query parameters
		QVariantMap m_sqlQueryParameters;

		// validates given sql query and its parameters
		bool validateSqlQuery(const QString& sqlQuery, const QVariantMap& sqlParameters) const;

		// normalizes parameters of the query: ':parameter' -> :parameter
		QString normalizeSqlQuery(const QString& sqlQuery, const QVariantMap& sqlParameters) const;

		// true if model is currently refreshing data
		bool m_isRefreshing{ false };

		// refresh data future watcher
		QFutureWatcher<LoadedData>* m_refreshFutureWatcher;

		// parses loaded sql data
		void parseSQLData();

		// task for full model refresh
		LoadedData fullDataRefresh(const QString& sqlQuery, const QVariantMap& sqlParameters);

		// task for partial model refresh
		LoadedData partialDataRefresh(const QString& sqlQuery, const QVariantMap& sqlParameters, const QList<int>& rows);

		// m_isRefreshing setter
		void setIsRefreshing(bool isRefreshing);

		// true if query is valid
		bool m_queryIsValid{ false };

		// m_queryIsValid setter
		void setQueryIsValid(bool queryIsValid);

	signals:
		// Q_PROPERTY changed signals
		void sqlQueryChanged();
		void sqlQueryParametersChanged();
		void isRefreshingChanged();
		void queryIsValidChanged();

		// emited when refresh starts
		void refreshStarted();

		// emited when refresh ends
		void refreshEnded(bool result);

		// emited every time model loads new rows
		void rowsLoaded(int rowsCount);
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLTABLEMODEL_H
