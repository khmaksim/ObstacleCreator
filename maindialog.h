#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QtGui/QStandardItemModel>
#include <QtCore/QSortFilterProxyModel>

namespace Ui {
    class MainDialog;
}
class ResultSearchAirfieldFilterModel;
class ObstracleFilterModel;
class QTableWidget;

class MainDialog : public QDialog
{
        Q_OBJECT

    public:
        explicit MainDialog(QWidget *parent = 0);
        ~MainDialog();

    private:
        Ui::MainDialog *ui;
        QString fileDatabase;
        QStandardItemModel *resultSearchModel;
        QStandardItemModel *obstracleModel;
        ResultSearchAirfieldFilterModel *filterSearchModel;
        ObstracleFilterModel *filterObstracleModel;
        int arpAirfield;

        void writeSettings();
        void readSettings();
        bool connectDatabase();
        void discconnetDatabase();
        void getListAirfield();
        void getListObstracleByAirfield();
        void clearTable(QTableWidget *table);
        void createFile(const QString &fileName);

    private slots:
        void searchAirfield(const QString&);
        void getInfoByAirfield(const QModelIndex&);
        void showSettings();
        void createFile();
        void filterObstacle();

    signals:
        void selectionAirfield(bool f=true);
};

#endif // MAINDIALOG_H
