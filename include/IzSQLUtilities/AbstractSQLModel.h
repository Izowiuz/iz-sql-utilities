#ifndef IZSQLUTILITIES_ABSTRACTSQLMODEL_H
#define IZSQLUTILITIES_ABSTRACTSQLMODEL_H

#include <atomic>
#include <memory>
#include <vector>

#include <QFutureWatcher>

#include "IzModels/AbstractItemModel.h"

#include "IzSQLUtilities/IzSQLUtilities_Global.h"
#include "SQLRow.h"

// TODO: przepisać normalniej funkcję validującą sql query i jego parametry
// TODO: może jakaś abstrakcyjny interfejs dla modelu danych?
// TODO: implementacja funkcjonalności abortLoading()
// TODO: implementacja funkcjonalności częściowego refresh'a
// TODO: sterowanie częstotliwością wysyłania sygnału rowsLoaded(int)

namespace IzSQLUtilities
{
	class LoadedSQLData;

	class IZSQLUTILITIESSHARED_EXPORT AbstractSQLModel : public IzModels::AbstractItemModel
	{
		Q_OBJECT
		Q_DISABLE_COPY(AbstractSQLModel)

		// sql query
		Q_PROPERTY(QString sqlQuery READ sqlQuery WRITE setSqlQuery NOTIFY sqlQueryChanged FINAL)

		// sql query parameters
		Q_PROPERTY(QVariantMap sqlQueryParameters READ sqlQueryParameters WRITE setSqlQueryParameters NOTIFY sqlQueryParametersChanged FINAL)

		// true if query is valid
		Q_PROPERTY(bool queryIsValid READ queryIsValid NOTIFY queryIsValidChanged)

	public:
		// types of data refresh
		enum class DataRefreshType : uint8_t {
			Full = 0,
			Partial
		};
		Q_ENUMS(DataRefreshType)

		// data refresh results
		enum class DataRefreshResult : uint8_t {
			Refreshed = 0,
			DatabaseError,
			QueryError
		};
		Q_ENUMS(DataRefreshType)

		using LoadedData = std::tuple<AbstractSQLModel::DataRefreshResult, AbstractSQLModel::DataRefreshType, std::shared_ptr<LoadedSQLData>>;

		// ctor
		AbstractSQLModel(QObject* parent = nullptr);

		// dtor
		virtual ~AbstractSQLModel() = default;

		// QAbstractItemModel interface start

		// header
		Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation = Qt::Horizontal, int role = Qt::DisplayRole) const override;
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

		// QAbstractItemModel interface end

		// custom model interface start

		// returns true if given index is in valid range for this model instance
		inline bool indexIsValid(int index) const
		{
			return index < static_cast<int>(m_data.size()) && index >= 0;
		};

		// returns column name for given column index
		// !WARNING! skips m_columnNameColumnAliasMap
		QString columnNameFromIndex(int index) const;

		// returns index for given column name
		int indexFromColumnName(const QString& column) const;

		// m_sqlQuery getter / setter
		QString sqlQuery() const;
		void setSqlQuery(const QString& sqlQuery);

		// m_sqlQueryParameters getter / setter
		QVariantMap sqlQueryParameters() const;
		void setSqlQueryParameters(const QVariantMap& sqlQueryParameters);

		// used to clear model data
		void clearData() override;

		// used to clear saved sql query and its parameters
		Q_INVOKABLE void clearQueryData();

		// used to refresh data, emits refreshStarted signal, sets sqlQuery and sqlQueryParameters
		Q_INVOKABLE void refreshData(const QString& sqlQuery, const QVariantMap& sqlParameters = {}, const QList<int>& rows = {});

		// used to refresh data, emits refreshStarted signal, uses set earlier sqlQuery and sqlQueryParameters members
		Q_INVOKABLE void refreshData();

		// used to add parameter to the query
		Q_INVOKABLE void addQueryParameter(const QString& parameter, const QVariant& value);

		// adds new row to sql data - returns true on success and false otherwise
		// defaultInitialize - if set to true missing columns will be initialized to its default type values
		// uniqueColumnValues - if set wil check if current set of data already has columns with given values - emits duplicateRow() on collision
		Q_INVOKABLE bool addRow(const QVariantMap& data, bool defaultInitialize = false, const QStringList& uniqueColumnValues = {});

		// removes row from sql data - returns true on success and false otherwise
		Q_INVOKABLE bool removeRow(int index);

		// returns true if executed query is different than the last one
		Q_INVOKABLE bool executedNewQuery() const;

		// returns index of the data row for which values from QVariantMap are equal or -1 if row was not found
		int findRow(const QVariantMap& columnValues) const;

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
		SQLRow& at(int index)
		{
			return *m_data[index];
		}

		// m_queryIsValid getter
		bool queryIsValid() const;

		// m_columnNameColumnAliasMap getter / setter
		QVariantMap columnNameColumnAliasMap() const;
		void setColumnNameColumnAliasMap(const QVariantMap& columnNameColumnAliasMap);

		// return types, as QMetaType::Type of given sql column or QMetaType::UnknownType if invalid insex was passed
		QMetaType::Type columnDataType(int index) const;

		// custom model interface end

		// AbstractItemModel interface start

		int roleNameToColumn(const QString& roleName) override;

		// AbstractItemModel interface end

	protected:
		// internal data getters
		std::vector<std::unique_ptr<SQLRow>>& internalData();
		const std::vector<std::unique_ptr<SQLRow>>& internalData() const;
		const QMap<int, QString>& indexColumnMap() const;
		const QHash<QString, int>& columnIndexMap() const;

		// allows for additiona data parsing during model refresh
		// executes post data load, right before endResetModel()
		virtual void additionalDataParsing(bool dataRefreshSucceeded);

	private:
		// internal data of the model
		std::vector<std::unique_ptr<SQLRow>> m_data;

		// sql column data types
		std::vector<QMetaType::Type> m_sqlDataTypes;

		// SQL column names -> indexes relations
		// contains valid values only post data load
		QHash<QString, int> m_columnIndexMap;

		// indexes -> SQL column names relations
		// contains valid values only post data load
		QMap<int, QString> m_indexColumnMap;

		// column names -> column aliases relations
		QVariantMap m_columnNameColumnAliasMap;

		// column aliases -> column names relations - set in setColumnNameColumnAliasMap()
		QVariantMap m_columnAliasColumnNameMap;

		// raw sql query
		QString m_sqlQuery;

		// last executed sql query
		QString m_lastQuery;

		// sql query parameters
		QVariantMap m_sqlQueryParameters;

		// validates given sql query and its parameters
		// passing silent = true silences errors
		bool validateSqlQuery(const QString& sqlQuery, const QVariantMap& sqlParameters, bool silent = false);

		// normalizes parameters of the query: ':parameter' -> :parameter
		QString normalizeSqlQuery(const QString& sqlQuery, const QVariantMap& sqlParameters) const;

		// refresh data future watcher
		QFutureWatcher<LoadedData>* m_refreshFutureWatcher;

		// parses loaded sql data
		void parseSQLData();

		// task for full model refresh
		LoadedData fullDataRefresh(const QString& sqlQuery, const QVariantMap& sqlParameters);

		// task for partial model refresh
		LoadedData partialDataRefresh(const QString& sqlQuery, const QVariantMap& sqlParameters, const QList<int>& rows);

		// true if query is valid
		bool m_queryIsValid{ false };

		// true if query is new
		bool m_newQuery{ true };

	signals:
		// Q_PROPERTY changed signals
		void sqlQueryChanged();
		void sqlQueryParametersChanged();
		void queryIsValidChanged();

		// emited when SQL query started
		void sqlQueryStarted();

		// emited when SQL query finished
		void sqlQueryReturned();

		// emited every time model loads new rows
		void rowsLoaded(int rowsCount);

		// emited when valid query was set
		void validQuerySet();

		// emited when, when adding new data, duplicate row was found
		void duplicateFound();
	};

}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_ABSTRACTSQLMODEL_H
