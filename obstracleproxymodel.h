#ifndef OBSTRACLEPROXYMODEL_H
#define OBSTRACLEPROXYMODEL_H

#include <QtCore/QSortFilterProxyModel>

class ObstracleProxyModel : public QSortFilterProxyModel
{
    public:
        ObstracleProxyModel(QObject *parent = 0);

    protected:
        bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

#endif // OBSTRACLEPROXYMODEL_H
