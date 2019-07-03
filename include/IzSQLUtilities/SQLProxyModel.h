#ifndef IZSQLUTILITIES_SQLPROXYMODEL_H
#define IZSQLUTILITIES_SQLPROXYMODEL_H

// TODO: zunifikować funkcje invertState / setState / removeState
// TOOO: RegExp w filtrach
// TODO: przyjrzeć się czy użycie m_clonedData nie powoduje memory leaków

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QFutureWatcher>
#include <QSharedPointer>
#include <QSortFilterProxyModel>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"

namespace IzSQLUtilities
{
	class SQLModel;
	class SQLQueryBuilder;
	class SQLDataContainer;

	class IZSQLUTILITIESSHARED_EXPORT SQLProxyModel : public QSortFilterProxyModel
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLProxyModel)

	public:
		// ctor
		explicit SQLProxyModel(QObject* parent = nullptr);

		// dtor
		~SQLProxyModel() = default;

		// returns source model for this proxy
		// if isInitializing = true is passed function will additionaly use checkSource()
		// it is used to silence warnings during setupSourceModel()
		Q_INVOKABLE SQLModel* sourceModel(bool isInitializing = false) const;

		// returns source row for given proxy row
		// returns -1 if invalid index was requested
		int sourceRow(int proxyRow) const;

		// returns proxy row for given source row
		// returns -1 if invalid index was requested
		int proxyRow(int sourceRow) const;

		// m_filterEnabled setter / getter
		bool getFilterEnabled() const;
		void setFilterEnabled(bool filterEnabled);

		// adds filter value [QVariant] for the field [QString]*/
		// replaces value for the already defined filter
		void addFilter(const QString& columnName, const QVariant& filterValue, bool exactValue = false);

		// removes filter for given column name
		void removeFilter(const QString& columnName);

		// clears all filters
		void clearFilters();

		// returns true if filtered model is empty
		bool isEmpty() const;

		// returns filter value for given columnName [QVariant] or null QString if filter was not found
		const QVariant getFilterValue(const QString& columnName) const;

		// TODO: tu chaos z nazewnictwem :O
		// functions used to multiedit model elements state for currently visible rows
		void invertState(bool isAdded);
		void setState(bool isAdded);
		void removeState(bool isAdded);
		void normalizeState(bool isAdded, bool state);

		// sorts model for given column
		void sortColumn(const QString& columnName, bool descending);

		// setups source model
		void virtual setupSource(SQLModel* source = nullptr);

		// checks for initialized source model and if proxy correctly connected to source's emits
		// returns true if source model was initialized and false otherwise
		bool checkSource(bool initialization = false) const;

	protected:
		// m_sourceModel setter
		void setSource(SQLModel* sourceModel);

		// setups connects
		void setupConnects();

		// QSortFilterProxyModel interface
		bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

	private:
		// source model handler
		SQLModel* m_sourceModel{ nullptr };

		// cached source model's data for concurrent filtering
		QVector<QSharedPointer<SQLDataContainer>> m_clonedData;

		// cached map of column -> index relations for concurrent filtering
		QHash<QString, int> m_cachedColumnIndexMap;

		// map of column -> index relations
		QHash<QString, int> m_columnIndexMap;

		// true if proxy connected to source's emits and false otherwise
		bool m_connectedToSource{ false };

		// if set to true proxy will filter SQL fields defined in filterFields QProperty
		bool m_filterEnabled{ false };

		// list of the currently appiled filters
		QHash<QString, QPair<QVariant, bool>> m_filters;

		// warper-like functions for the soucre model signals
		void onSourceModelDataAboutToBeChanged(int row, const QString& columnName, const QVariant& newFieldValue);
		void onSourceModelRowAboutToBeAdded(int row, const std::vector<QVariant>& fieldValues);

		// called when QFuture filters all model data
		void onDataFiltered();

		// called when proxy filter changes
		// used to start QFuture and QFutureWatcher
		void onFilterChangeRequested();

		// filter watcher
		QFutureWatcher<QSet<int>>* m_filterWatcher;

		// filtered source indexes
		QSet<int> m_filteredIndexes;

		// callback for sourceModel's refreshCompleted signal
		void onSourceRefreshCompleted();

		// callback for sourceModel's refreshStarted signal
		void onSourceRefreshStarted();

		// caches source model's data
		void cacheSourceData();

		// clears cached source model's data
		void clearCachedSourceData();

	signals:
		// emited when filterValue was changed
		void filterChanged(QVariant filterValue);

		// emited when model starts filtering data
		void filteringStarted();

		// emited when model stops filtering data
		void filteringEnded();

		// emited when data was sorted
		void dataSorted(QString columnName, bool descending);

		// emited when mosel will be sorted
		void aboutToSort(QString columnName, bool descending);
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLPROXYMODEL_H
