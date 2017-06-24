#ifndef RESULTSEARCHAIRFIELDFILTERMODEL_H
#define RESULTSEARCHAIRFIELDFILTERMODEL_H

#include <QtCore/QSortFilterProxyModel>

class ResultSearchAirfieldFilterModel : public QSortFilterProxyModel
{
        Q_OBJECT

    public:
        ResultSearchAirfieldFilterModel(QObject *parent = 0);

    protected:
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};

#endif // RESULTSEARCHAIRFIELDFILTERMODEL_H
