#ifndef IZSQLUTILITIES_SQLMODEL_H
#define IZSQLUTILITIES_SQLMODEL_H

/*
	Abstract SQL Model

	TODO: parametry dla refreshFailed()
	TODO: poprawić dataChanged() w setElementPropertyValue() tak, żeby działał także dla kolumn w widokach C++
	TODO: zaimplementować więcej funkcji QAbstractItemModel
	TODO: prawdopodobnie zrobić tak, żeby addElement() pozwalał dodawać puste parametry
	TODO: całkowicie usunąć albo poprawić emisje sygnału refreshAborted()
	TODO: poprawić raportowanie loadera
	TODO: pododawać dataAboutToBeChanged() tam gdzie to ma sens
	TODO: zoptymalizować funkcję addRow() i addRows()
*/

#include "IzSQLUtilities/IzSQLUtilities_Global.h"

#include <QAbstractItemModel>
#include <QSet>
#include <QSharedPointer>

#include "IzSQLUtilities/IzSQLUtilities_Enums.h"

namespace IzSQLUtilities
{
	class SQLData;
	class SQLQueryBuilder;
	class SQLDataLoader;
	class SQLDataContainer;

	class IZSQLUTILITIESSHARED_EXPORT SQLModel : public QAbstractItemModel
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLModel)

	public:
		// ctor
		explicit SQLModel(QObject* parent = nullptr);

		// dtor
		~SQLModel() = default;

		// QAbstractItemModel interface START*/

		// returns model row count
		int rowCount(const QModelIndex& index = {}) const override;

		// returns model column count
		int columnCount(const QModelIndex& index = {}) const override;

		// returns model data at index*/
		QVariant data(const QModelIndex& index, int role) const override;

		// returns index at row, column with parent
		QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;

		// returns element's parent
		QModelIndex parent(const QModelIndex& index = {}) const override;

		// role names - obligatory for QML based models
		QHash<int, QByteArray> roleNames() const override;

		// QAbstractItemModel interface END*/

		// refreshes model data using query, refresh is possible only when model is in idle state
		// asynchronousLoad is passed to the SQLDataLoader instance to indicate data load behaviour
		// if null or no QString is passed model uses query constructed by the query builder
		// when raportProgress parameter is set to true loader will emit total number of rows to load and will be reporting loading progress
		void refreshData(const QString& sqlQuery = {}, bool reportProgress = false, bool asynchronousLoad = true);

		// used to refresh only selected rows [QList<int>]
		// when raportProgress parameter is set to true loader will emit total number of rows to load and will be reporting loading progress
		void refreshRows(const QList<int>& rows, bool reportProgress = true);

		// returns field value [QVariant] for chosen row [int] for given columnName [QString]*/
		// or invalid QVariant if field or row was not found
		QVariant getFieldValue(int row, const QString& columnName) const;
		QVariant getFieldValue(int row, const char* columnName) const;

		// returns row's column names and their values
		QVariantMap getRowData(int row) const;

		// sets field value [QVariant] for chosen row [int] for given column [QString]*/
		// returns true on success and false otherwise
		bool setFieldValue(int row, const QString& columnName, const QVariant& fieldValue);
		bool setFieldValue(int row, const char* columnName, const QVariant& fieldValue);

		// sets given [QVariantMap {fieldName - fieldValue}] values for all model rows
		void setRowsData(const QVariantMap& fieldValues);

		// removes row [int] from the model
		// returns true on success and false otherwise
		bool removeRow(int row);

		// returns true if model is currently loading data
		bool isInLoadProcess();

		// clears model; removes internal data; emits necessary signals [begin & end model reset]
		void clearData();

		// returns current model size
		int dataSize() const;

		// returns handler to the query builder
		SQLQueryBuilder* getQueryBuilder() const;

		// function used to abort model refresh
		void requestAbort();

		// returns load status of the model
		ModelLoadStatus getLoadStatus() const;

		// returns true if the column [QString] in the row [int] is dirty and false otherwise
		bool fieldIsDirty(int row, const QString& columnName) const;

		// cleans field [QString] in the row [int]
		// returns true on success and false otherwise
		bool cleanField(int row, const QString& columnName);

		// returns true if the row [int] is dirty and false otherwise
		bool rowIsDirty(int row) const;

		// cleans row [int]
		// returns true on success and false otherwise
		bool cleanRow(int row);

		// returns true if the model is dirty and false otherwise
		// model is considered dirty if any filed of its rows was changed
		// ! OR if one of them has toBeRemoved or isAdded set to true !
		bool isDirty() const;

		// returns count of changed rows
		int changedRowsCount() const;

		// cleans model
		// returns true on success and false otherwise
		bool cleanData();

		// creates new row and appends it to the model
		// function creates row with given SQL field values
		// controlFields [QStringList] - list of field to check for duplicates before adding
		// if replaceToAdd is set to true old field values, according to the fieldsToReplace list, will be replaced with the new ones
		// returns true on success and false otherwise
		bool addRow(const QVariantMap& fieldValues, const QStringList& controlFields = {});

		// adds multiple rows to the model
		// see addRow
		bool addRows(const QList<QPair<QVariantMap, QStringList>>& rows);

		// returns toBeRemoved for a given row
		bool toBeRemoved(int row) const;

		// flips toBeRemoved for the given row [int]
		void flipToBeRemoved(int row);

		// returns isAdded for a given row
		bool isAdded(int row) const;

		// flips isAdded for the given row [int]
		void flipIsAdded(int row);

		// prints row [int] data
		void debug_rowData(int row) const;

		// prints model data
		void debug_modelData() const;

		// returns column's names as returned from sql query
		QStringList getSQLColumnNames() const;

		// returns column index map
		QHash<QString, int> getColumnIndexMap() const;

		// returns row of the first index for which value(columnName) == value
		int findFirstRowIndex(const QString& columnName, const QVariant& fieldValue) const;

		// m_identityColumn getter / setter
		QString getIdentityColumn() const;
		void setIdentityColumn(const QString& columnName);

		// m_identityTable getter / setter
		QString getIdentityTable() const;
		void setIdentityTable(const QString& tableName);

		// m_defaultFieldValues setter / getter
		QVariantMap getDefaultFieldValues() const;
		void setDefaultFieldValues(const QVariantMap& defaultFieldValues);

		// m_replaceOnAdd setter / getter
		bool getReplaceFieldsOnAdd() const;
		void setReplaceFieldsOnAdd(bool replaceFieldsOnAdd);

		// m_fieldsToReplace setter / getter
		QStringList getFieldsToReplace() const;
		void setFieldsToReplace(const QStringList& fieldsToReplace);

		// m_allowContinuousRefreshes setter / getter
		bool getAllowContinuousRefreshes() const;
		void setAllowContinuousRefreshes(bool allowContinuousRefreshes);

		// returns index of given column from sql query
		int getColumnIndex(const QString& columnName) const;

		// returns all unique values for given column names
		QVariantMap getUniqueFieldValues(const QStringList& columnNames) const;

		// returns all changed rows
		QList<QVariantMap> getChangedRowsData() const;

		// clones of model's data
		QVector<QSharedPointer<IzSQLUtilities::SQLDataContainer>> cloneData() const;

		// loader's database type getter / setter
		DatabaseType getDatabaseType() const;
		void setDatabaseType(DatabaseType databaseType);

		// loader's databaseParameters getter / setter
		QVariantMap getDatabaseParameters() const;
		void setDatabaseParameters(const QVariantMap& databaseParameters);

	private:
		// if true during refresh operation model will cancel current refresh operation and start a new one
		bool m_allowContinuousRefreshes{ false };

		// if true during abort loader will emit partially loaded data
		bool m_emitOnAbort{ true };

		// sql data loader
		SQLDataLoader* m_dataLoader;

		// processes data emited by the SQLDataLoader
		bool processData(QSharedPointer<IzSQLUtilities::SQLData> data);

		// internal model data
		QVector<QSharedPointer<SQLDataContainer>> m_modelData;

		// map of column -> index relations
		QHash<QString, int> m_columnIndexMap;

		// map of index -> column relations
		QHash<int, QString> m_indexColumnMap;

		// sql column names returned from sql data loader
		QStringList m_sqlColumnNames;

		// true if model is currently loading data
		bool m_isLoadingData{ false };

		// load status
		ModelLoadStatus m_loadStatus{ ModelLoadStatus::NOT_INITIALIZED };

		// internal SQLQueryBuilder instance
		SQLQueryBuilder* m_queryBuilder;

		// holds default values for fields not provided in addRow()
		QVariantMap m_defaultDatafieldValues;

		// used with partial refresh to determine which column in which table used as SQL identity
		QString m_identityColumn;
		QString m_identityTable;

		// replaceFieldsOnAdd member
		bool m_replaceFieldsOnAdd{ true };

		// fieldsToReplace member
		QStringList m_fieldsToReplace;

		// model's dirty rows
		QSet<QSharedPointer<SQLDataContainer>> m_dirtyRows;

		// functions used to add / remove / clean rows to / from QSet of dirty ones and, if necessary, emit correct signals
		void updateRowState(QSharedPointer<SQLDataContainer> row);
		void clearDirtyRows();

		// cached role names
		mutable QHash<int, QByteArray> m_cachedRoleNames;

	signals:
		// emited when model starts refreshing
		// partialRefresh indicates if refresh was partial or complete
		void refreshStarted(bool partialRefresh = false);

		// emited when model completes load operation
		// partialRefresh indicates if refresh was partial or complete
		void refreshCompleted(bool partialRefresh = false);

		// emited when model completes partial refresh operation
		// refreshed indicates how many rows were refreshed
		// lost indicates how many rows were lost due to query filters
		void partialRefreshOutcome(int refreshed, int lost);

		// emited when processData() function reports error from SQLDataLoader
		void refreshFailed();

		// emited when refresh operation was aborted
		void refreshAborted();

		// signal used to abort refresh operation
		void abortRefresh();

		// emited when model data was cleared
		void dataCleared();

		// emited when query counting was requested, passes information about number of rows loaded
		void rowsLoaded(int count);

		// emited when duplicate row was detected in addRow()
		void duplicate();

		// emited when models changes its state between clean / dirty
		void modelStateChanged(bool isDirty);

		// emited when data is about to be changed
		// currently emited only in setFieldValue functions
		void dataAboutToBeChanged(int row, QString columnName, QVariant newFieldValue);

		// emited when row is about to be added
		// currently emited only in setFieldValue functions
		void rowAboutToBeAdded(int row, std::vector<QVariant> fieldValues);
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLMODEL_H
