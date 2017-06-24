#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    readSettings();

    connect(ui->pathDatabaseToolButton, SIGNAL(clicked(bool)), this, SLOT(showSelectPath()));
    connect(ui->outputPathToolButton, SIGNAL(clicked(bool)), this, SLOT(showSelectPath()));
    connect(ui->closeButton, SIGNAL(clicked(bool)), this, SLOT(close()));
}

SettingsDialog::~SettingsDialog()
{
    writeSettings();
    delete ui;
}

void SettingsDialog::showSelectPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select path"), QDir::homePath());
    if (!path.isEmpty()) {
        if (sender()->objectName().contains("Database"))
            ui->pathToDatabaseLineEdit->setText(QDir::toNativeSeparators(path));
        else
            ui->outputPathLineEdit->setText(QDir::toNativeSeparators(path));
    }
}

void SettingsDialog::writeSettings()
{
    QSettings settings;

    settings.beginGroup("geometry");
    settings.setValue("height", this->height());
    settings.setValue("width", this->width());
    settings.setValue("headerStatePathFileTable", ui->pathFileTableWidget->horizontalHeader()->saveState());
    settings.setValue("geometryPathFileTable", ui->pathFileTableWidget->saveGeometry());
    settings.endGroup();
    settings.beginGroup("connectDatabase");
    settings.setValue("pathToDatabase", ui->pathToDatabaseLineEdit->text());
    settings.endGroup();
    settings.beginGroup("createFile");
    settings.setValue("outputPath", ui->outputPathLineEdit->text());
    QMap<QString, QVariant> descriptionNameFile;
    for (int row = 0; row < ui->pathFileTableWidget->rowCount(); row++)
        descriptionNameFile[ui->pathFileTableWidget->item(row, 0)->text()] = ui->pathFileTableWidget->item(row, 1)->text();
    settings.setValue("descriptionNameFiles", descriptionNameFile);
    settings.endGroup();
    settings.endGroup();
}

void SettingsDialog::readSettings()
{
    QSettings settings;

    settings.beginGroup("geometry");
    int height = settings.value("height", 400).toInt();
    int width = settings.value("width", 600).toInt();
    ui->pathFileTableWidget->horizontalHeader()->restoreState(settings.value("headerStatePathFileTable").toByteArray());
    ui->pathFileTableWidget->restoreGeometry(settings.value("geometryPathFileTable").toByteArray());
    settings.endGroup();
    settings.beginGroup("connectDatabase");
    ui->pathToDatabaseLineEdit->setText(settings.value("pathToDatabase").toString());
    settings.endGroup();
    settings.beginGroup("createFile");
    ui->outputPathLineEdit->setText(settings.value("outputPath").toString());
    if (!settings.contains("descriptionNameFiles"))
        setDefaultValue();
    QMap<QString, QVariant> descriptionNameFile = settings.value("descriptionNameFiles").toMap();
    QMap<QString, QVariant>::const_iterator constIt = descriptionNameFile.constBegin();
    while (constIt != descriptionNameFile.constEnd()) {
        ui->pathFileTableWidget->insertRow(0);
        ui->pathFileTableWidget->setItem(0, 0, new QTableWidgetItem(constIt.key()));
        ui->pathFileTableWidget->setItem(0, 1, new QTableWidgetItem(constIt.value().toString()));
        ++constIt;
    }
    settings.endGroup();

    this->resize(width, height);
}

void SettingsDialog::setDefaultValue()
{
    QFile file(":/descriptionFileName.txt");
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        QString line;
        while (stream.readLineInto(&line)) {
            QStringList keyValue = line.split(";");
            ui->pathFileTableWidget->insertRow(0);
            ui->pathFileTableWidget->setItem(0, 0, new QTableWidgetItem(keyValue.at(0)));
            ui->pathFileTableWidget->setItem(0, 1, new QTableWidgetItem(keyValue.at(1)));
        }
    }
    file.close();
}
