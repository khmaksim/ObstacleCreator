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
#include "settingsdialog.h"

MainDialog::MainDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDialog)
{
    ui->setupUi(this);

    readSettings();

    ui->thr1FilterLineEdit->setValidator(new QDoubleValidator());
    ui->thr2FilterLineEdit->setValidator(new QDoubleValidator());
    ui->conditionFilterLineEdit->setValidator(new QDoubleValidator());

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

    connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), this, SLOT(searchAirfield(QString)));
    connect(ui->thr1FilterLineEdit, SIGNAL(editingFinished()), this, SLOT(searchObstacle()));
    connect(ui->thr2FilterLineEdit, SIGNAL(editingFinished()), this, SLOT(searchObstacle()));
    connect(ui->conditionFilterLineEdit, SIGNAL(editingFinished()), this, SLOT(searchObstacle()));
    connect(ui->resultSearchTableView, SIGNAL(clicked(QModelIndex)), this, SLOT(getInfoByAirfield(QModelIndex)));
    connect(this, SIGNAL(selectionAirfield(bool)), ui->filterGroupBox, SLOT(setEnabled(bool)));
    connect(ui->settingsButton, SIGNAL(clicked(bool)), this, SLOT(showSettings()));
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
    settings.setValue("headerStateResultFilterTable", ui->obstacleTableWidget->horizontalHeader()->saveState());
    settings.setValue("geometryResultFilterTable", ui->obstacleTableWidget->saveGeometry());
    settings.setValue("splitter", ui->splitter->saveState());
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
    ui->obstacleTableWidget->horizontalHeader()->restoreState(settings.value("headerStateResultFilterTable").toByteArray());
    ui->obstacleTableWidget->restoreGeometry(settings.value("geometryResultFilterTable").toByteArray());
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
    settings.endGroup();
    settings.beginGroup("conectDatabase");
    pathToDatabase = settings.value("pathToDatabase").toString();
    settings.endGroup();

    if(pathToDatabase.isEmpty())
        showSettings();

    this->resize(width, height);
}

bool MainDialog::connectDatabase()
{
    if (QFileInfo(pathToDatabase).exists()) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
        db.setHostName("localhost");
        db.setDatabaseName(QString("Driver={Microsoft Access Driver (*.mdb)};DSN='';DBQ=%1").arg(pathToDatabase));
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

    query.prepare("SELECT airfield.NAMEI, airfield.ID, airfield.id_aero, MIN(tr1) as minTr1, MIN(tr2) as minTr2 FROM "
                  "(SELECT airfield.NAMEI, airfield.ID, airfield.id_aero, MIN(tr1.H_threshold) as tr1, MAX(tr2.H_threshold) as tr2 "
                  "FROM AERODROMI airfield, VPP vpp, torezh tr1, torezh tr2 "
                  "WHERE airfield.id_aero = ? AND airfield.id_aero = vpp.id_arpt AND vpp.id_vpp = tr1.id_vpp "
                  "AND vpp.id_vpp = tr2.id_vpp "
                  "GROUP BY airfield.NAMEI, airfield.ID, airfield.id_aero, tr1.id_vpp, tr2.id_vpp) "
                  "GROUP BY airfield.NAMEI, airfield.ID, airfield.id_aero");
    query.addBindValue(filterSearchModel->data(filterSearchModel->index(index.row(), 2)).toString());
    query.exec();
    if (query.first()) {
        ui->nameAirdieldlineEdit->setText(query.value(0).toString());
        ui->icaoAirfieldLineEdit->setText(query.value(1).toString());
        ui->arpAirfieldLineEdit->setText(query.value(2).toString());
        ui->thr1AirfieldLineEdit->setText(QString::number(query.value(3).toFloat(), 'f', 1));
        ui->thr2AirfieldLineEdit->setText(QString::number(query.value(4).toFloat(), 'f', 1));
        emit selectionAirfield();
    }
}

void MainDialog::searchObstacle()
{
    clearTable(ui->obstacleTableWidget);

    QSqlQuery query;

    query.prepare("SELECT TOP 10 pr.CODA, pr.TYPPREP, pr.LAT, pr.LON, pr.Habs "
                  "FROM PREPARPT pr, AERODROMI airfield "
                  "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND pr.Habs > ?");
    query.addBindValue(ui->arpAirfieldLineEdit->text().toInt());
    query.addBindValue(qMin(qMin(ui->thr1FilterLineEdit->text().toDouble() + ui->thr1AirfieldLineEdit->text().toDouble(),
                                 ui->thr2FilterLineEdit->text().toDouble() + ui->thr2AirfieldLineEdit->text().toDouble()),
                            ui->conditionFilterLineEdit->text().toDouble()));
    if (!query.exec())
        qDebug() << query.lastError().text();
    while (query.next()) {
        ui->obstacleTableWidget->insertRow(0);
        ui->obstacleTableWidget->setItem(0, 0, new QTableWidgetItem(query.value(0).toString()));
        ui->obstacleTableWidget->setItem(0, 1, new QTableWidgetItem(query.value(1).toString()));
        ui->obstacleTableWidget->setItem(0, 2, new QTableWidgetItem(query.value(2).toString()));
        ui->obstacleTableWidget->setItem(0, 3, new QTableWidgetItem(query.value(3).toString()));
        ui->obstacleTableWidget->setItem(0, 4, new QTableWidgetItem(QString::number(query.value(4).toDouble(), 'f', 2)));
    }
}

void MainDialog::clearTable(QTableWidget *table)
{
    while (table->rowCount() > 0)
        table->removeRow(0);
}

void MainDialog::showSettings()
{
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

void MainDialog::createFile()
{

}
