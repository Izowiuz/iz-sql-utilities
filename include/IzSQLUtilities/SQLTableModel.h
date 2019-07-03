#ifndef IZSQLUTILITIES_SQLTABLEMODEL_H
#define IZSQLUTILITIES_SQLTABLEMODEL_H

#include "AbstractSQLModel.h"
#include "IzSQLUtilities/IzSQLUtilities_Global.h"

namespace IzSQLUtilities
{
	class IZSQLUTILITIESSHARED_EXPORT SQLTableModel : public AbstractSQLModel
	{
		Q_OBJECT
		Q_DISABLE_COPY(SQLTableModel)

	public:
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

		// ctors
		explicit SQLTableModel(QObject* parent = nullptr);

		// dtor
		~SQLTableModel() = default;

		// QAbstractItemModel interface start

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

		// QAbstractItemModel interface stop

		// AbstractSQLModel interface start

		void additionalDataParsing(bool dataRefreshSucceeded) override;

		// AbstractSQLModel interface end
	};
}   // namespace IzSQLUtilities

#endif   // IZSQLUTILITIES_SQLTABLEMODEL_H
