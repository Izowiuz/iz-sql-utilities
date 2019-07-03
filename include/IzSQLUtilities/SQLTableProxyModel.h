#ifndef IZSQLUTILITIES_SQLTABLEPROXYMODEL_H
#define IZSQLUTILITIES_SQLTABLEPROXYMODEL_H

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QFutureWatcher>
#include <QRegularExpression>
#include <QSet>
#include <QSortFilterProxyModel>

class QItemSelectionModel;

// TODO: należałoby pozbyć się QItemSelectionModel'u z tego poziomu
// TODO: asynchroniczne sortowanie

namespace IzSQLUtilities
{
	class SQLTableModel;

	class IZSQLUTILITIESSHARED_EXPORT SQLTableProxyModel : public QSortFilterProxyModel
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLTableProxyModel)

		// source model for this proxy
		Q_PROPERTY(SQLTableModel* source READ source CONSTANT FINAL)

		// true if model is currently filtering data
		Q_PROPERTY(bool isFiltering READ isFiltering NOTIFY isFilteringChanged FINAL)

	public:
		explicit SQLTableProxyModel(QObject* parent = nullptr);

		// QSortFilterProxyModel interface start

		QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
		void setSourceModel(QAbstractItemModel* sourceModel) override;

		// QSortFilterProxyModel end

		// QAbstractItemModel interface start

		Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation = Qt::Horizontal, int role = Qt::DisplayRole) const override;

		// QAbstractItemModel interface end

		// returns internal m_sourceModel
		Q_INVOKABLE IzSQLUtilities::SQLTableModel* source() const;

		// m_selectionModel getter / setter
		QItemSelectionModel* selectionModel() const;
		void setSelectionModel(QItemSelectionModel* selectionModel);

		// changes visibility of given column
		void changeColumnVisibilitiy(int column, bool visibility);

		// starts data filtering process
		Q_INVOKABLE void filterData();

		// m_isFiltering getter
		bool isFiltering() const;

		// adds filter for column
		void addColumnFilter(int column, const QRegularExpression& filter);

		// removes filter for column
		void removeColumnFilter(int column);

		// clears column filters
		void clearColumnFilters();

		// m_excludedColumns getter / setter
		QSet<int> excludedColumns() const;
		void setExcludedColumns(const QSet<int>& excludedColumns);

		// returns source row for given proxy row
		// WARNING: proxyColumn set to 0!
		Q_INVOKABLE int sourceRow(int proxyRow) const;

		// returns source column for given proxy column
		// WARNING: proxyRow set to 0!
		Q_INVOKABLE int sourceColumn(int proxyColumn) const;

		// returns proxy row for given source row
		// WARNING: sourceColumn set to 0!
		Q_INVOKABLE int proxyRow(int sourceRow) const;

		// returns proxy column for given source column
		// WARNING: sourceRow set to 0!
		Q_INVOKABLE int proxyColumn(int sourceColumn) const;

		// returns source model index
		Q_INVOKABLE QModelIndex sourceIndex(int proxyRow, int proxyColumn) const;

		// returns proxy model index
		Q_INVOKABLE QModelIndex proxyIndex(int sourceRow, int sourceColumn) const;

		// m_hiddenColumns getter / setter
		QSet<int> hiddenColumns() const;
		void setHiddenColumns(const QSet<int> &hiddenColumns);

	protected:
		// QSortFilterProxyModel start

		bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
		bool filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const override;

		// QSortFilterProxyModel end

	private:
		// internal source model
		SQLTableModel* m_sourceModel;

		// internal handler to the selectionModel
		// WARNING: this is used as a hack to implement selection functionality under QML
		QItemSelectionModel* m_selectionModel{ nullptr };

		// set of hidden columns
		QSet<int> m_hiddenColumns;

		// set of globally hidden columns
		QSet<int> m_excludedColumns;

		// filter data future watcher
		QFutureWatcher<QSet<int>>* m_filterFutureWatcher;

		// parses filtered data
		void onDataFiltered();

		// true if model is currently filtering data
		bool m_isFiltering{ false };

		// set of filtered indexes
		QSet<int> m_filteredIndexes;

		// column filters
		QHash<int, QRegularExpression> m_filters;

		// cached column filters
		// WARNING: this member is used during filter operation
		QHash<int, QRegularExpression> m_cachedFilters;

		// true if filter were applied
		bool m_filtersApplied{ false };

	signals:
		// Q_PROPERTY *Changed signals
		void isFilteringChanged();
	};

}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLTABLEPROXYMODEL_H
