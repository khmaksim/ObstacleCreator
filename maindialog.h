#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QtGui/QStandardItemModel>
#include <QtCore/QSortFilterProxyModel>

namespace Ui {
    class MainDialog;
}
class ResultSearchAirfieldFilterModel;
class QTableWidget;

class MainDialog : public QDialog
{
        Q_OBJECT

    public:
        explicit MainDialog(QWidget *parent = 0);
        ~MainDialog();

    private:
        Ui::MainDialog *ui;
        QString pathToDatabase;
        QStandardItemModel *resultSearchModel;
        ResultSearchAirfieldFilterModel *filterSearchModel;

        void writeSettings();
        void readSettings();
        bool connectDatabase();
        void getListAirfield();
        void clearTable(QTableWidget *table);

    private slots:
        void searchAirfield(const QString&);
        void getInfoByAirfield(const QModelIndex&);
        void searchObstacle();
        void showSettings();

    signals:
        void selectionAirfield(bool f=true);
};

#endif // MAINDIALOG_H
