#pragma once

#include "IzSQLUtilities/AbstractSQLModel.h"
#include "IzSQLUtilities_Global.h"

namespace IzSQLUtilities
{
    class IZSQLUTILITIESSHARED_EXPORT SQLListModel : public AbstractSQLModel
    {
        Q_OBJECT
        Q_DISABLE_COPY(SQLListModel)

    public:
        // ctor
        SQLListModel(QObject* parent = nullptr);

        // QAbstractItemModel interface start

        QVariant data(const QModelIndex& index, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::DisplayRole) override;

        // QAbstractItemModel interface end

        // AbstractSQLModel interface start

        void additionalDataParsing(bool dataRefreshSucceeded) override;

        // AbstractSQLModel interface end
    };
}   // namespace IzSQLUtilities
