#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QtGui/QStandardItemModel>
#include <QtCore/QSortFilterProxyModel>

namespace Ui {
    class MainDialog;
}
class ResultSearchAirfieldFilterModel;

class MainDialog : public QDialog
{
        Q_OBJECT

    public:
        explicit MainDialog(QWidget *parent = 0);
        ~MainDialog();

    private:
        Ui::MainDialog *ui;
        QString pathToFileDatabase;
        QStandardItemModel *resultSearchModel;
        ResultSearchAirfieldFilterModel *filterSearchModel;

        void writeSettings();
        void readSettings();
        bool connectDatabase();
        void getListAirfield();

    private slots:
        void searchAirfield(const QString&);
        void getInfoByAirfield(const QModelIndex&);
};

#endif // MAINDIALOG_H
