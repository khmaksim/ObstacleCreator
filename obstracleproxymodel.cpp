#include "obstracleproxymodel.h"

ObstracleProxyModel::ObstracleProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool ObstracleProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &/*sourceParent*/) const
{
    return sourceRow < 10;
}
