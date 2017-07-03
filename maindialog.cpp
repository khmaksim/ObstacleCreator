#include "maindialog.h"
#include "ui_maindialog.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QSettings>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QMessageLogger>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QListWidget>
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
    filterObstacleModel = new ObstracleFilterModel(this);
    filterObstacleModel->setSourceModel(obstracleModel);
    ObstracleProxyModel *proxyObstracleModel = new ObstracleProxyModel(this);
    proxyObstracleModel->setSourceModel(filterObstacleModel);
    ui->obstacleTableView->setModel(proxyObstracleModel);
    ui->obstacleTableView->setColumnHidden(5, true);
    ui->obstacleTableView->setColumnHidden(6, true);

    resultSearchModel = new QStandardItemModel(this);
    resultSearchModel->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Name 2") << tr("-"));
    filterSearchModel = new ResultSearchAirfieldFilterModel(this);
    filterSearchModel->setSourceModel(resultSearchModel);
    ui->resultSearchTableView->setModel(filterSearchModel);
    ui->resultSearchTableView->setColumnHidden(2, true);

    if (!connectDatabase())
        QMessageBox::warning(this, tr("Warning"), tr("Failed to connect to the database"));

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
    connect(ui->createButton, SIGNAL(clicked(bool)), this, SLOT(create()));
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
    settings.setValue("headerStateObstacleTable", ui->obstacleTableView->horizontalHeader()->saveState());
    settings.setValue("geometryObstacleTable", ui->obstacleTableView->saveGeometry());
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
    ui->obstacleTableView->horizontalHeader()->restoreState(settings.value("headerStateObstacleTable").toByteArray());
    ui->obstacleTableView->restoreGeometry(settings.value("geometryObstacleTable").toByteArray());
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
        if (db.open()) {
            QMessageLogger(0, 0, 0).info("Connect database");
            QMessageLogger(0, 0, 0).debug() << "file database" << fileDatabase;
            return true;
        }
        QMessageLogger(0, 0, 0).warning("Failed to connect to the database");
        QMessageLogger(0, 0, 0).debug() << db.lastError().text();
//        qDebug() << db.lastError().text();
    }
    return false;
}

void MainDialog::discconnetDatabase()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.close();
    QMessageBox::information(this, tr("Information"), tr("The database connection is broken."));
    QMessageLogger(0, 0, 0).info("The database connection is broken");
}

void MainDialog::getListAirfield()
{
    QSqlQuery query;

    query.exec("SELECT id_aero, NAMEI, NAMEI2 FROM AERODROMI ORDER BY NAMEI, NAMEI2");
    if (query.lastError().isValid()) {
        QMessageLogger(0, 0, 0).warning() << query.lastError().text();
        QMessageLogger(0, 0, 0).debug() << query.lastQuery();
    }
    while (query.next()) {
        resultSearchModel->appendRow(QList<QStandardItem*>()
                                     << new QStandardItem(QString("%1%2%3")
                                                          .arg(query.value(1).toString())
                                                          .arg(!query.value(2).toString().isEmpty() ? "/" : "")
                                                          .arg(query.value(2).toString()))
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

    query.prepare("SELECT airfield.HABS FROM AERODROMI airfield WHERE airfield.id_aero = ?");
    query.addBindValue(filterSearchModel->data(filterSearchModel->index(index.row(), 2)).toString());
    if (!query.exec()) {
        QMessageLogger(0, 0, 0).warning() << query.lastError().text();
        QMessageLogger(0, 0, 0).debug() << query.lastQuery();
        return;
    }
    if (query.first() && query.value(0).toString().isEmpty())
        QMessageBox::warning(this, tr("Warning"), tr("The airport has no value height."));

    query.prepare("SELECT * FROM VPP vpp WHERE vpp.id_arpt = ?");
    query.addBindValue(filterSearchModel->data(filterSearchModel->index(index.row(), 2)).toString());
    if (!query.exec()) {
        QMessageLogger(0, 0, 0).warning() << query.lastError().text();
        QMessageLogger(0, 0, 0).debug() << query.lastQuery();
        return;
    }
    if (!query.first()) {
        QMessageBox::warning(this, tr("Warning"), tr("The selected airfield not the runway."));
        return;
    }

    query.prepare("SELECT * FROM VPP vpp, torezh tr WHERE vpp.id_arpt = ? AND vpp.id_vpp = tr.id_vpp");
    query.addBindValue(filterSearchModel->data(filterSearchModel->index(index.row(), 2)).toString());
    if (!query.exec()) {
        QMessageLogger(0, 0, 0).warning() << query.lastError().text();
        QMessageLogger(0, 0, 0).debug() << query.lastQuery();
        return;
    }
    if (!query.first()) {
        QMessageBox::warning(this, tr("Warning"), tr("The selected airfield not the ends."));
        return;
    }

//    query.prepare("SELECT airfield.NAMEI, airfield.IDRUS1, airfiled.id_aero, MIN(tr1) as minTr1, MIN(tr2) as minTr2 "
//                  "FROM (SELECT airfield.NAMEI, airfield.IDRUS1, airfiled.id_aero, MIN(tr1.H_threshold) as tr1, MAX(tr2.H_threshold) as tr2 "
//                  "FROM AERODROMI airfield, VPP vpp, torezh tr1, torezh tr2 "
//                  "WHERE airfield.id_aero = ? AND airfield.id_aero = vpp.id_arpt AND vpp.id_vpp = tr1.id_vpp "
//                  "AND vpp.id_vpp = tr2.id_vpp "
//                  "GROUP BY airfield.NAMEI, airfield.IDRUS1, airfiled.id_aero, tr1.id_vpp, tr2.id_vpp) "
//                  "GROUP BY airfield.NAMEI, airfield.IDRUS1, airfiled.id_aero");
    query.prepare("SELECT airfield.NAMEI, airfield.IDRUS1, airfield.HABS, airfield.id_aero, MIN(tr1.H_threshold), MAX(tr2.H_threshold) "
                  "FROM AERODROMI airfield, VPP vpp, torezh tr1, torezh tr2 "
                  "WHERE airfield.id_aero = ? AND airfield.id_aero = vpp.id_arpt AND vpp.id_vpp = tr1.id_vpp "
                  "AND vpp.id_vpp = tr2.id_vpp AND vpp.pr_stat_vpp = 1 "
                  "GROUP BY airfield.NAMEI, airfield.IDRUS1, airfield.HABS, airfield.id_aero");
    query.addBindValue(filterSearchModel->data(filterSearchModel->index(index.row(), 2)).toInt());
    if (!query.exec()) {
        QMessageLogger(0, 0, 0).warning() << query.lastError().text();
        QMessageLogger(0, 0, 0).debug() << query.lastQuery();
        return;
    }
    if (query.first()) {
        ui->nameAirdieldlineEdit->setText(query.value(0).toString());
        ui->icaoAirfieldLineEdit->setText(query.value(1).toString());
        ui->arpAirfieldLineEdit->setText(QString::number(qRound(query.value(2).toDouble())));
        idAirfield = query.value(3).toInt();
        ui->thr1AirfieldLineEdit->setText(QString::number(query.value(4).toFloat(), 'f', 2));
        ui->thr2AirfieldLineEdit->setText(QString::number(query.value(5).toFloat(), 'f', 2));
        emit selectionAirfield();
        getListObstracleByAirfield();
    }
}

void MainDialog::filterObstacle()
{
    filterObstacleModel->setFilterValue(2, QVariant(qMin(((ui->thr1FilterLineEdit->text().toDouble() + ui->thr1AirfieldLineEdit->text().toDouble()),
                                                           (ui->thr2FilterLineEdit->text().toDouble() + ui->thr2AirfieldLineEdit->text().toDouble())),
                                                          ui->conditionFilterLineEdit->text().toDouble())));
}

void MainDialog::showSettings()
{
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

void MainDialog::create()
{
    QSettings settings;

    settings.beginGroup("createFile");
    QString outputPath = settings.value("outputPath").toString();

    if (!settings.contains("nameFiles"))
        showSettings();
    QStringList nameFile = settings.value("nameFiles").toStringList();
    settings.endGroup();

    double condition = ui->conditionFilterLineEdit->text().toDouble();
    double thr1 = ui->thr1FilterLineEdit->text().toDouble() + ui->thr1AirfieldLineEdit->text().toDouble();
    double thr2 = ui->thr1FilterLineEdit->text().toDouble() + ui->thr2AirfieldLineEdit->text().toDouble();
    bool mAbsolute = ui->mAbsoluteRadioButton->isChecked();
    bool feetAbsolute = ui->feetAbsoluteRadioButton->isChecked();
    bool mRelative = ui->mRelativeRadioButton->isChecked();
    bool feetRelative = ui->feetRelativeRadioButton->isChecked();
    double relationValue;
    bool showAbsolute;
    bool showRelative;

    QString fileName;
    for (int i = 0; i < nameFile.count(); i++) {
        relationValue = 0;
        showAbsolute = showRelative = true;
        fileName = outputPath + "/" + nameFile.at(i);
        switch (i) {
            case 0:
                if (!mAbsolute)
                    continue;
                filterObstacleModel->setFilterValue(5, "2");
                showRelative = false;
                break;
            case 1:
                if (!feetAbsolute)
                    continue;
                filterObstacleModel->setFilterValue(5, "2");
                showRelative = false;
                break;
            case 2:
                if (!mAbsolute)
                    continue;
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                showRelative = false;
                break;
            case 3:
                if (!feetAbsolute)
                    continue;
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                showRelative = false;
                break;
            case 4:
                if (!mAbsolute)
                    continue;
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                showRelative = false;
                break;
            case 5:
                if (!feetAbsolute)
                    continue;
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                showRelative = false;
                break;
            case 6:
                if (!mAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, condition);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = condition;
                break;
            case 7:
                if (!mAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr1);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr1;
                break;
            case 8:
                if (!mAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr2);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr2;
                break;
            case 9:
                if (!feetAbsolute || !feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, condition);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = condition;
                break;
            case 10:
                if (!feetAbsolute || !feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr1);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr1;
                break;
            case 11:
                if (!feetAbsolute || !feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr2);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr2;
                break;
            case 12:
                if (!feetAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, condition);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = condition;
                break;
            case 13:
                if (!feetAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr1);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr1;
                break;
            case 14:
                if (!feetAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr2);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr2;
                break;
            case 15:
                if (!mAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, condition);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = condition;
                break;
            case 16:
                if (!mAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr1);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = thr1;
                break;
            case 17:
                if (!mAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr2);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = thr2;
                break;
            case 18:
                if (!feetAbsolute || !feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, condition);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = condition;
                break;
            case 19:
                if (!feetAbsolute || !feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr1);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = thr1;
                break;
            case 20:
                if (!feetAbsolute || !feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr2);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = thr2;
                break;
            case 21:
                if (!feetAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, condition);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = condition;
                break;
            case 22:
                if (!feetAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr1);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = thr1;
                break;
            case 23:
                if (!feetAbsolute || !mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr2);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "1");
                relationValue = thr2;
                break;
            case 24:
                if (!mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, condition);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = condition;
                showAbsolute = false;
                break;
            case 25:
                if (!mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr1);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr1;
                showAbsolute = false;
                break;
            case 26:
                if (!mRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr2);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr2;
                showAbsolute = false;
                break;
            case 27:
                if (!feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, condition);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = condition;
                showAbsolute = false;
                break;
            case 28:
                if (!feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr1);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr1;
                showAbsolute = false;
                break;
            case 29:
                if (!feetRelative)
                    continue;
                filterObstacleModel->setFilterValue(4, thr2);
                filterObstacleModel->setFilterValue(5, "1?");
                filterObstacleModel->setFilterValue(6, "");
                relationValue = thr2;
                showAbsolute = false;
                break;
            default:
                continue;
                break;
        }
        createFile(fileName, showAbsolute, showRelative, relationValue);
    }
    showReport();
}

void MainDialog::createFile(const QString &fileName, bool showAbsolute, bool showRelative, double relationValue)
{
    QFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream stream(&file);
        for (int row = 0; row < filterObstacleModel->rowCount(); row++) {
            stream << filterObstacleModel->data(filterObstacleModel->index(row, 2)).toString().simplified() << endl;
            stream << filterObstacleModel->data(filterObstacleModel->index(row, 3)).toString().simplified() << endl;
            if (ui->mAbsoluteRadioButton->isChecked() && showAbsolute)
                stream << filterObstacleModel->data(filterObstacleModel->index(row, 4)).toDouble() << endl;
            else if (ui->feetAbsoluteRadioButton->isChecked() && showAbsolute)
                stream << filterObstacleModel->data(filterObstacleModel->index(row, 4)).toDouble() * 3.28 << " '" << endl;
            else
                stream << "" << endl;
            if (ui->mRelativeRadioButton->isChecked() && showRelative)
                stream << (filterObstacleModel->data(filterObstacleModel->index(row, 4)).toDouble() - relationValue) << endl;
            else if (ui->feetRelativeRadioButton && showRelative)
                stream << (filterObstacleModel->data(filterObstacleModel->index(row, 4)).toDouble() - relationValue) * 3.28 << " '" << endl;
            else
                stream << "" << endl;
            stream << "1" << endl;
        }
    }
    else {
        QMessageLogger(0, 0, 0).warning() << file.errorString();
        QMessageLogger(0, 0, 0).debug() << file.fileName();
    }
    file.close();
    createReport(file);
}

void MainDialog::getListObstracleByAirfield()
{
    ui->obstacleTableView->model()->removeRows(0, ui->obstacleTableView->model()->rowCount());

    QSqlQuery query;

    query.prepare("SELECT pr.CODA, pr.TYPPREP, pr.LAT, pr.LON, pr.Habs,  pr.art_nat, pr.MARKER FROM PREPARPT pr, AERODROMI airfield "
                  "WHERE airfield.id_aero = ? AND airfield.id_aero = pr.CODA ORDER BY pr.Habs DESC");
    query.addBindValue(idAirfield);
    if (!query.exec()) {
        QMessageLogger(0, 0, 0).warning() << query.lastError().text();
        QMessageLogger(0, 0, 0).debug() << query.lastQuery();
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

void MainDialog::createReport(const QFile &file)
{
    QListWidgetItem *item = new QListWidgetItem(QDir::toNativeSeparators(file.fileName()));
    if (file.size() == 0)
        item->setTextColor(QColor(Qt::red));
    reportItems.append(item);
}

void MainDialog::showReport()
{
    QListWidget *listWidget = new QListWidget();

    QList<QListWidgetItem*>::const_iterator constIt = reportItems.constBegin();
    for (; constIt != reportItems.constEnd(); ++constIt)
        listWidget->addItem(*constIt);

    listWidget->setWindowTitle(this->windowTitle() + " - Report");
    listWidget->setWindowFlags(listWidget->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    listWidget->show();
}
