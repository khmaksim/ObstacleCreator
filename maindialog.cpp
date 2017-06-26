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
#include "obstraclefiltermodel.h"
#include "obstracleproxymodel.h"
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

    obstracleModel = new QStandardItemModel(this);
    obstracleModel->setHorizontalHeaderLabels(QStringList() << tr("ID") << tr("Name") << tr("LAT") << tr("LON") << tr("H"));
    filterObstracleModel = new ObstracleFilterModel(this);
    filterObstracleModel->setSourceModel(obstracleModel);
    ObstracleProxyModel *proxyObstracleModel = new ObstracleProxyModel(this);
    proxyObstracleModel->setSourceModel(filterObstracleModel);
    ui->obstacleTableView->setModel(proxyObstracleModel);
    ui->obstacleTableView->setColumnHidden(5, true);
    ui->obstacleTableView->setColumnHidden(6, true);

    resultSearchModel = new QStandardItemModel(this);
    resultSearchModel->setHorizontalHeaderLabels(QStringList() << tr("Name 1") << tr("Name 2") << tr("-"));
    filterSearchModel = new ResultSearchAirfieldFilterModel(this);
    filterSearchModel->setSourceModel(resultSearchModel);
    ui->resultSearchTableView->setModel(filterSearchModel);
    ui->resultSearchTableView->setColumnHidden(2, true);

    if (!connectDatabase()) {
        QMessageBox::critical(this, tr("Critical"), tr("Failed to connect to the databse"));
    }
    getListAirfield();

    connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), this, SLOT(searchAirfield(QString)));
    connect(ui->thr1FilterLineEdit, SIGNAL(editingFinished()), this, SLOT(filterObstacle()));
    connect(ui->thr2FilterLineEdit, SIGNAL(editingFinished()), this, SLOT(filterObstacle()));
    connect(ui->conditionFilterLineEdit, SIGNAL(editingFinished()), this, SLOT(filterObstacle()));
    connect(ui->resultSearchTableView, SIGNAL(clicked(QModelIndex)), this, SLOT(getInfoByAirfield(QModelIndex)));
    connect(this, SIGNAL(selectionAirfield(bool)), ui->filterGroupBox, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(selectionAirfield(bool)), ui->createButton, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(selectionAirfield(bool)), this, SLOT(filterObstacle()));
    connect(ui->settingsButton, SIGNAL(clicked(bool)), this, SLOT(showSettings()));
    connect(ui->createButton, SIGNAL(clicked(bool)), this, SLOT(createFile()));
}

MainDialog::~MainDialog()
{
//    discconnetDatabase();
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
    settings.setValue("headerStateObsracleTable", ui->obstacleTableView->horizontalHeader()->saveState());
    settings.setValue("geometryObstracleTable", ui->obstacleTableView->saveGeometry());
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
    ui->obstacleTableView->horizontalHeader()->restoreState(settings.value("headerStateObstracleTable").toByteArray());
    ui->obstacleTableView->restoreGeometry(settings.value("geometryObstracleTable").toByteArray());
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
    settings.endGroup();
    settings.beginGroup("connectDatabase");
    fileDatabase = settings.value("fileDatabase").toString();

    if(fileDatabase.isEmpty()) {
        showSettings();
        fileDatabase = settings.value("fileDatabase").toString();
    }
    settings.endGroup();



    this->resize(width, height);
}

bool MainDialog::connectDatabase()
{
    if (QFileInfo(fileDatabase).exists()) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
        db.setHostName("localhost");
        db.setDatabaseName(QString("Driver={Microsoft Access Driver (*.mdb)};DSN='';DBQ=%1").arg(fileDatabase));
        if (db.open())
            return true;
        qDebug() << db.lastError().text();
    }
    return false;
}

void MainDialog::discconnetDatabase()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.close();
    QMessageBox::information(this, tr("Information"), tr("The database connection is broken."));
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
        arpAirfield = query.value(2).toInt();
        ui->thr1AirfieldLineEdit->setText(QString::number(query.value(3).toFloat(), 'f', 1));
        ui->thr2AirfieldLineEdit->setText(QString::number(query.value(4).toFloat(), 'f', 1));
        emit selectionAirfield();
        getListObstracleByAirfield();
    }
}

void MainDialog::filterObstacle()
{
    filterObstracleModel->setFilterValue(2, QVariant(qMin((ui->thr1FilterLineEdit->text().toDouble() + ui->thr1AirfieldLineEdit->text().toDouble(),
                                                           ui->thr2FilterLineEdit->text().toDouble() + ui->thr2AirfieldLineEdit->text().toDouble()),
                                                          ui->conditionFilterLineEdit->text().toDouble())));
//    QSqlQuery query;

//    query.prepare("SELECT TOP 10 pr.CODA, pr.TYPPREP, pr.LAT, pr.LON, pr.Habs "
//                  "FROM PREPARPT pr, AERODROMI airfield "
//                  "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND pr.Habs > ?");
//    query.addBindValue(arpAirfield);
//    query.addBindValue(qMin(qMin(ui->thr1FilterLineEdit->text().toDouble() + ui->thr1AirfieldLineEdit->text().toDouble(),
//                                 ui->thr2FilterLineEdit->text().toDouble() + ui->thr2AirfieldLineEdit->text().toDouble()),
//                            ui->conditionFilterLineEdit->text().toDouble()));
//    if (!query.exec()) {
//        qDebug() << query.lastError().text();
//        return;
//    }
//    while (query.next()) {
//        ui->obstacleTableWidget->insertRow(0);
//        ui->obstacleTableWidget->setItem(0, 0, new QTableWidgetItem(query.value(0).toString()));
//        ui->obstacleTableWidget->setItem(0, 1, new QTableWidgetItem(query.value(1).toString()));
//        ui->obstacleTableWidget->setItem(0, 2, new QTableWidgetItem(query.value(2).toString()));
//        ui->obstacleTableWidget->setItem(0, 3, new QTableWidgetItem(query.value(3).toString()));
//        ui->obstacleTableWidget->setItem(0, 4, new QTableWidgetItem(QString::number(query.value(4).toDouble(), 'f', 2)));
//    }
}

void MainDialog::showSettings()
{
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

void MainDialog::createFile()
{
    QSettings settings;

    settings.beginGroup("createFile");
    QString outputPath = settings.value("outputPath").toString();
    if (!settings.contains("descriptionNameFiles"))
        showSettings();

    QList<QVariant> descriptionNameFile = settings.value("descriptionNameFiles").toMap().values();
    settings.endGroup();

//    double condition = ui->conditionFilterLineEdit->text().toDouble();
//    double thr1 = ui->thr1FilterLineEdit->text().toDouble();
//    double thr2 = ui->thr1FilterLineEdit->text().toDouble();

    QSqlQuery query;
    for (int i = 0; i < descriptionNameFile.count(); i++) {
        createFile(outputPath + "/" + descriptionNameFile.at(i).toString());

//            switch (i) {
//                case 0:
//                    if (ui->mAbsoluteRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 2 "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toString() << endl;
//                            stream << "" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 1:
//                    if (ui->feetAbsoluteRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 2 "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 2:
//                    if (ui->mAbsoluteRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 3:
//                    if (ui->feetAbsoluteRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 4:
//                    if (ui->mAbsoluteRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = 1 "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 5:
//                    if (ui->feetAbsoluteRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = 1 "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 6:
//                    if (ui->mAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(condition);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - condition << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 7:
//                    if (ui->mAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr1);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - thr1 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 8:
//                    if (ui->mAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr2);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - thr2 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 9:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(condition);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << endl;
//                            stream << "H" << (query.value(2).toDouble() - condition) * 3.28 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 10:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr1);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "H" << (query.value(2).toDouble() - thr1)  * 3.28 << " '" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 11:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr2);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "H" << (query.value(2).toDouble() - thr2)  * 3.28 << " '" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 12:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(condition);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "H" << (query.value(2).toDouble() - condition) << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 13:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr1);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "H" << (query.value(2).toDouble() - thr1) << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 14:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr2);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "H" << query.value(2).toDouble() - thr2 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 15:
//                    if (ui->mAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(condition);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - condition << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 16:
//                    if (ui->mAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr1);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - thr1 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 17:
//                    if (ui->mAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr2);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - thr2 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 18:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(condition);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - condition << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 19:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr1);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - thr1 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 20:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr2);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() << endl;
//                            stream << "H" << query.value(2).toDouble() - thr2 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 21:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(condition);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "H" << query.value(2).toDouble() - condition << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 22:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr1);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "H" << query.value(2).toDouble() - thr1 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 23:
//                    if (ui->feetAbsoluteRadioButton->isChecked() && ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '1' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr2);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << "H" << query.value(2).toDouble() * 3.28 << " '" << endl;
//                            stream << "H" << query.value(2).toDouble() - thr2 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 24:
//                    if (ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(condition);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << endl;
//                            stream << "H" << query.value(2).toDouble() - condition << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 25:
//                    if (ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr1);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << endl;
//                            stream << "H" << query.value(2).toDouble() - thr1 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 26:
//                    if (ui->mRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr2);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << endl;
//                            stream << "H" << query.value(2).toDouble() - thr2 << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 27:
//                    if (ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(condition);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << endl;
//                            stream << "H" << (query.value(2).toDouble() - condition) * 3.28 << " '" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 28:
//                    if (ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr1);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << endl;
//                            stream << "H" << (query.value(2).toDouble() - thr1) * 3.28 << " '" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                case 29:
//                    if (ui->feetRelativeRadioButton->isChecked()) {
//                        query.prepare("SELECT pr.LAT, pr.LON, pr.Habs "
//                                      "FROM PREPARPT pr, AERODROMI airfield "
//                                      "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA AND art_nat = 1 OR art_nat = '' "
//                                      "AND MARKER = '' AND pr.Habs > ? "
//                                      "ORDER BY pr.Habs DESC");
//                        query.addBindValue(arpAirfield);
//                        query.addBindValue(thr2);
//                        if (!query.exec()) {
//                            qDebug() << query.lastError().text();
//                            return;
//                        }
//                        while (query.next()) {
//                            stream << query.value(0).toString().simplified() << endl;
//                            stream << query.value(1).toString().simplified() << endl;
//                            stream << endl;
//                            stream << "H" << (query.value(2).toDouble() - thr2) * 3.28 << " '" << endl;
//                            stream << "1" << endl;
//                        }
//                    }
//                    else
//                        file.remove();
//                    break;
//                default:
//                    break;
//            }
//            break;
//        }
//        file.close();
    }
}

void MainDialog::createFile(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream stream(&file);
        stream << endl;
    }
    file.close();
}

void MainDialog::getListObstracleByAirfield()
{
    QSqlQuery query;

    query.prepare("SELECT pr.CODA, pr.TYPPREP, pr.LAT, pr.LON, pr.Habs,  pr.art_nat, pr.MARKER FROM PREPARPT pr, AERODROMI airfield "
                  "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA ORDER BY pr.Habs DESC");
    query.addBindValue(arpAirfield);
    if (!query.exec()) {
        qDebug() << query.lastError().text();
        return;
    }
    while (query.next()) {
        obstracleModel->appendRow(QList<QStandardItem*>()
                                  << new QStandardItem(QString::number(query.value(0).toInt()))
                                  << new QStandardItem(query.value(1).toString())
                                  << new QStandardItem(query.value(2).toString())
                                  << new QStandardItem(query.value(3).toString())
                                  << new QStandardItem(QString::number(query.value(4).toDouble(), 'f', 2))
                                  << new QStandardItem(query.value(5).toString())
                                  << new QStandardItem(query.value(6).toString()));
    }
    ui->obstacleTableView->setColumnHidden(5, true);
    ui->obstacleTableView->setColumnHidden(6, true);
}
