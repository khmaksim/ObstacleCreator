#include "obstraclefiltermodel.h"
#include <QDebug>

ObstracleFilterModel::ObstracleFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{

}

bool ObstracleFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    bool result = true;

    if (filterValues.contains(4))
        result &= sourceModel()->data(sourceModel()->index(sourceRow, 4, sourceParent)).toDouble() > filterValues.value(4).toDouble();
    if (filterValues.contains(5))
        result &= sourceModel()->data(sourceModel()->index(sourceRow, 5, sourceParent)).toString().contains(QRegExp(filterValues.value(5).toString()));
    if (filterValues.contains(6))
        result &= sourceModel()->data(sourceModel()->index(sourceRow, 6, sourceParent)).toString().contains(QRegExp(filterValues.value(6).toString()));
    return result;
}

void ObstracleFilterModel::setFilterValue(int column, QVariant value)
{
    if (column < sourceModel()->columnCount())
        filterValues.insert(column, value);
}
