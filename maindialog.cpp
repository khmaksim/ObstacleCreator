#include "maindialog.h"
#include "ui_maindialog.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtWidgets/QMessageBox>
#include <QDebug>
#include "resultsearchairfieldfiltermodel.h"

MainDialog::MainDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDialog)
{
    ui->setupUi(this);

    readSettings();
    pathToFileDatabase = "D:/projects/Qt/ObstacleCreator-build-Desktop_Qt_5_8_0_MinGW_32bit-Debug/ADB.mdb";

    resultSearchModel = new QStandardItemModel(this);
    resultSearchModel->setHorizontalHeaderLabels(QStringList() << tr("Name 1") << tr("Name 2"));
    filterSearchModel = new ResultSearchAirfieldFilterModel(this);
    filterSearchModel->setSourceModel(resultSearchModel);
    ui->resultSearchTableView->setColumnHidden(2, true);
    ui->resultSearchTableView->setModel(filterSearchModel);

    if (!connectDatabase()) {
        QMessageBox::critical(this, tr("Critical"), tr("Failed to connect to the databse"));
    }
    getListAirfield();
//    connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), filterSearchModel, SLOT(setFilterWildcard(QString)));
//    connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), filterSearchModel, SLOT(setFilterRegExp(QString)));
//    connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), filterSearchModel, SLOT(setFilterFixedString(QString)));
    connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), this, SLOT(searchAirfield(QString)));
    connect(ui->resultSearchTableView, SIGNAL(clicked(QModelIndex)), this, SLOT(getInfoByAirfield(QModelIndex)));
}

MainDialog::~MainDialog()
{
    writeSettings();
    delete ui;
}

void MainDialog::writeSettings()
{
    QSettings settings;

    settings.beginGroup("geometry");
    settings.setValue("height", this->height());
    settings.setValue("width", this->width());
    settings.setValue("headerStateResultSearchTable", ui->resultSearchTableView->horizontalHeader()->saveState());
    settings.setValue("geometryResultSearchTable", ui->resultSearchTableView->saveGeometry());
    settings.setValue("headerStateResultFilterTable", ui->resultFilterTableWidget->horizontalHeader()->saveState());
    settings.setValue("geometryResultFilterTable", ui->resultFilterTableWidget->saveGeometry());
    settings.endGroup();
}

void MainDialog::readSettings()
{
    QSettings settings;

    settings.beginGroup("geometry");
    int height = settings.value("height", 400).toInt();
    int width = settings.value("width", 600).toInt();
    ui->resultSearchTableView->horizontalHeader()->restoreState(settings.value("headerStateResultSearchTable").toByteArray());
    ui->resultSearchTableView->restoreGeometry(settings.value("geometryResultSearchTable").toByteArray());
    ui->resultFilterTableWidget->horizontalHeader()->restoreState(settings.value("headerStateResultFilterTable").toByteArray());
    ui->resultFilterTableWidget->restoreGeometry(settings.value("geometryResultFilterTable").toByteArray());
    settings.endGroup();

    this->resize(width, height);
}

bool MainDialog::connectDatabase()
{
    if (QFileInfo(pathToFileDatabase).exists()) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
        db.setHostName("localhost");
        db.setDatabaseName(QString("Driver={Microsoft Access Driver (*.mdb)};DSN='';DBQ=%1").arg(pathToFileDatabase));
        if (db.open())
            return true;
        qDebug() << db.lastError().text();
    }
    return false;
}

void MainDialog::getListAirfield()
{
    QSqlQuery query;

    query.exec("SELECT id_aero, NAMEI, NAMEI2 FROM AERODROMI ORDER BY NAMEI, NAMEI2");
    while (query.next()) {
        resultSearchModel->appendRow(QList<QStandardItem*>()
                                     << new QStandardItem(query.value(1).toString())
                                     << new QStandardItem(query.value(2).toString())
                                     << new QStandardItem(query.value(0).toString()));
    }
}

void MainDialog::searchAirfield(const QString &pattern)
{
    filterSearchModel->setFilterRegExp(QRegExp("^" + pattern, Qt::CaseInsensitive));
}

void MainDialog::getInfoByAirfield(const QModelIndex &index)
{
    QSqlQuery query;

    query.prepare("SELECT NAMEI, ID, id_aero FROM AERODROMI WHERE id_aero = ?");
    query.addBindValue(resultSearchModel->data(resultSearchModel->index(index.row(), 2)).toString());
    query.exec();
    if (query.first()) {
        ui->nameAirdieldlineEdit->setText(query.value(0).toString());
        ui->icaoAirfieldLineEdit->setText(query.value(1).toString());
        ui->arpAirFieldLineEdit->setText(query.value(2).toString());
    }
}
