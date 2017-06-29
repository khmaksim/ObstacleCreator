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
class QListWidgetItem;
class QFile;

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
        ObstracleFilterModel *filterObstacleModel;
        int idAirfield;
        QList<QListWidgetItem*> reportItems;

        void writeSettings();
        void readSettings();
        bool connectDatabase();
        void discconnetDatabase();
        void getListAirfield();
        void getListObstracleByAirfield();
        void createFile(const QString &fileName, bool showAbsolute, bool showRelative, double);
        void createReport(const QFile&);
        void showReport();

    private slots:
        void searchAirfield(const QString&);
        void getInfoByAirfield(const QModelIndex&);
        void showSettings();
        void create();
        void filterObstacle();

    signals:
        void selectionAirfield(bool f=true);
};

#endif // MAINDIALOG_H
