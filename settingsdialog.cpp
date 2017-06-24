#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QtWidgets/QFileDialog>
#include <QtCore/QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(ui->pathDatabaseToolButton, SIGNAL(clicked(bool)), this, SLOT(showSelectPath()));
    connect(ui->outputPathToolButton, SIGNAL(clicked(bool)), this, SLOT(showSelectPath()));
    connect(ui->closeButton, SIGNAL(clicked(bool)), this, SLOT(close()));
}

SettingsDialog::~SettingsDialog()
{
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
    settings.beginGroup("conectDatabase");
    settings.setValue("pathToDatabase", ui->pathToDatabaseLineEdit->text());
    settings.endGroup();
    settings.beginGroup("createFile");
    settings.setValue("outputPath", ui->outputPathLineEdit->text());
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
    settings.beginGroup("conectDatabase");
    ui->pathToDatabaseLineEdit->setText(settings.value("pathToDatabase").toString());
    settings.endGroup();
    settings.beginGroup("createFile");
    ui->outputPathLineEdit->setText(settings.value("outputPath").toString());
    settings.endGroup();

    this->resize(width, height);
}
