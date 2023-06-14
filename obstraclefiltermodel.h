#ifndef OBSTRACLEFILTERMODEL_H
#define OBSTRACLEFILTERMODEL_H

#include <QtCore/QSortFilterProxyModel>

class ObstracleFilterModel : public QSortFilterProxyModel
{
        Q_OBJECT

    public:
        ObstracleFilterModel(QObject *parent = 0);

        void setFilterValue(int column, QVariant value);

    protected:
        bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

    private:
        QMap<int, QVariant> filterValues;
};

#endif // OBSTRACLEFILTERMODEL_H
