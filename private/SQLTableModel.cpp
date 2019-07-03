#include "IzSQLUtilities/SQLTableModel.h"

#include <QDebug>

IzSQLUtilities::SQLTableModel::SQLTableModel(QObject* parent)
	: AbstractSQLModel(parent)
{

}

QVariant IzSQLUtilities::SQLTableModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) {
		return {};
	}
	switch (static_cast<SQLTableModel::SQLTableModelRoles>(role)) {
	case SQLTableModel::SQLTableModelRoles::DisplayData:
		return internalData()[index.row()]->columnValue(index.column());
	default:
		return {};
	}
}

bool IzSQLUtilities::SQLTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid()) {
		qCritical() << "Got invalid index:" << index << "in setData() function.";
		return true;
	}
	// TODO: for now, only EditRole can be changed, small hack
	if ((role == Qt::DisplayRole || role == Qt::EditRole) && data(index, Qt::DisplayRole) != value) {
		auto res = internalData()[index.row()]->setColumnValue(index.column(), value);
		if (res) {
			emit dataChanged(index, index, { Qt::DisplayRole });
		}
		return res;
	}
	return false;
}

Qt::ItemFlags IzSQLUtilities::SQLTableModel::flags(const QModelIndex& index) const
{
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}
	return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool IzSQLUtilities::SQLTableModel::insertRows(int row, int count, const QModelIndex& parent)
{
	beginInsertRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endInsertRows();
	return {};
}

bool IzSQLUtilities::SQLTableModel::insertColumns(int column, int count, const QModelIndex& parent)
{
	beginInsertColumns(parent, column, column + count - 1);
	// FIXME: Implement me!
	endInsertColumns();
	return {};
}

bool IzSQLUtilities::SQLTableModel::removeRows(int row, int count, const QModelIndex& parent)
{
	beginRemoveRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endRemoveRows();
	return {};
}

bool IzSQLUtilities::SQLTableModel::removeColumns(int column, int count, const QModelIndex& parent)
{
	beginRemoveColumns(parent, column, column + count - 1);
	// FIXME: Implement me!
	endRemoveColumns();
	return {};
}

void IzSQLUtilities::SQLTableModel::additionalDataParsing(bool dataRefreshSucceeded)
{
	if (dataRefreshSucceeded) {
		QHash<int, QByteArray> rn;
		rn.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::DisplayData), QByteArrayLiteral("displayData"));
		rn.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::IsAdded), QByteArrayLiteral("isAdded"));
		rn.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::ToBeRemoved), QByteArrayLiteral("toBeRemoved"));
		rn.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::IsDirty), QByteArrayLiteral("isDirty"));
		rn.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::IsValid), QByteArrayLiteral("isValid"));
		rn.insert(static_cast<int>(SQLTableModel::SQLTableModelRoles::IsSelected), QByteArrayLiteral("isSelected"));
		cacheRoleNames(rn);
	} else {
		clearCachedRoleNames();
	}
}
