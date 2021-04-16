#include "IzSQLUtilities/SQLListModel.h"

#include <QDebug>

IzSQLUtilities::SQLListModel::SQLListModel(QObject* parent)
    : AbstractSQLModel(parent)
{
}

QVariant IzSQLUtilities::SQLListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (role >= Qt::UserRole) {
        if (role >= attachedRoleLowerLimit()) {
            return attachedRoleValue(index, role);
        }

        switch (static_cast<AbstractItemModelRoles>(role)) {
        case AbstractItemModelRoles::IsChanged:
            return indexWasChanged(index);
        case AbstractItemModelRoles::IsAdded:
            return indexWasAdded(index);
        default:
            return internalData()[index.row()]->columnValue(role - Qt::UserRole);
        }
    }

    return {};
}

bool IzSQLUtilities::SQLListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid()) {
        qWarning() << "Got an invalid model index.";

        return false;
    }

    // attached role
    if (role >= attachedRoleLowerLimit()) {
        emit dataAboutToBeChanged(index, index, { role });
        setAttachedRoleValue(index, value, role);

        return true;
    }

    // 'normal' role
    if (data(index, role) != value) {
        emit dataAboutToBeChanged(index, index, { role });
        auto res = internalData()[index.row()]->setColumnValue(roleNameToColumn(roleToRoleName(role)), value);

        if (res) {
            emit dataChanged(index, index, { role });
        }

        return res;
    }

    return false;
}

void IzSQLUtilities::SQLListModel::additionalDataParsing(bool dataRefreshSucceeded)
{
    if (dataRefreshSucceeded) {
        QHash<int, QByteArray> rn;
        QMapIterator<int, QString> it(indexColumnMap());

        while (it.hasNext()) {
            it.next();

            if (rn.contains(it.key() + Qt::UserRole)) {
                qWarning() << "Got duplicated column:" << it.value() << "from query. Column will be skipped.";
            } else {
                rn.insert(Qt::UserRole + it.key(), it.value().toUtf8());
            }
        }
        cacheRoleNames(rn);
    } else {
        clearCachedRoleNames();
    }
}
