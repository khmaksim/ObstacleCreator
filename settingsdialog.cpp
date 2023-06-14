#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtCore/QSettings>
#include <QtCore/QTextStream>
#include <QtCore/QMessageLogger>
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    readSettings();
    ui->pathFileTableWidget->setSortingEnabled(false);

    connect(ui->pathDatabaseToolButton, SIGNAL(clicked(bool)), this, SLOT(showSelectFileDatabase()));
    connect(ui->outputPathToolButton, SIGNAL(clicked(bool)), this, SLOT(showSelectOutputPath()));
    connect(ui->closeButton, SIGNAL(clicked(bool)), this, SLOT(close()));
    connect(ui->pathFileTableWidget, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showInputValue(QModelIndex)));
}

SettingsDialog::~SettingsDialog()
{
    writeSettings();
    delete ui;
}

void SettingsDialog::showSelectOutputPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select output path the file"), QDir::homePath());
    if (!path.isEmpty())
        ui->outputPathLineEdit->setText(QDir::toNativeSeparators(path));
}

void SettingsDialog::showSelectFileDatabase()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select file database"), QDir::homePath(), tr("Access file database (*.mdb)"));
    if (!file.isEmpty())
        ui->fileDatabaseLineEdit->setText(QDir::toNativeSeparators(file));
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
    settings.setValue("fileDatabase", ui->fileDatabaseLineEdit->text());
    settings.endGroup();
    settings.beginGroup("createFile");
    settings.setValue("outputPath", ui->outputPathLineEdit->text());
    QStringList descriptionFile;
    QStringList nameFile;
    for (int row = 0; row < ui->pathFileTableWidget->rowCount(); row++) {
        descriptionFile << ui->pathFileTableWidget->item(row, 0)->text();
        nameFile << ui->pathFileTableWidget->item(row, 1)->text();
    }
    settings.setValue("descriptionFiles", descriptionFile);
    settings.setValue("nameFiles", nameFile);
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
    ui->fileDatabaseLineEdit->setText(settings.value("pathToDatabase").toString());
    settings.endGroup();
    settings.beginGroup("createFile");
    ui->outputPathLineEdit->setText(settings.value("outputPath").toString());
    if (!settings.contains("descriptionNameFiles"))
        setDefaultValue();
    int row = 0;
    QStringList descriptionFile = settings.value("descriptionFiles").toStringList();
    QStringList nameFile = settings.value("nameFiles").toStringList();
    QStringList::const_iterator constIt = descriptionFile.constBegin();
    while (constIt != descriptionFile.constEnd()) {
        ui->pathFileTableWidget->insertRow(row);
        ui->pathFileTableWidget->setItem(row, 0, new QTableWidgetItem((*constIt)));
        ui->pathFileTableWidget->setItem(row, 1, new QTableWidgetItem(nameFile.at(row)));
        row++;
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
        int row = 0;
        while (stream.readLineInto(&line)) {
            QStringList keyValue = line.split(";");
            ui->pathFileTableWidget->insertRow(row);
            ui->pathFileTableWidget->setItem(row, 0, new QTableWidgetItem(keyValue.at(0)));
            ui->pathFileTableWidget->setItem(row, 1, new QTableWidgetItem(keyValue.at(1)));
            row++;
        }
    }
    else {
        QMessageLogger(0, 0, 0).warning() << file.errorString();
        QMessageLogger(0, 0, 0).debug() << file.fileName();
    }
    file.close();
}

void SettingsDialog::showInputValue(const QModelIndex &index)
{
    bool ok;
    Qt::WindowFlags flags = Qt::Dialog | Qt::WindowContextHelpButtonHint;

    QString value = QInputDialog::getText(this, tr("Enter file name"), tr("File name:"), QLineEdit::Normal,
                                         ui->pathFileTableWidget->item(index.row(), 1)->text(), &ok, flags);
    if (ok && !value.isEmpty())
        ui->pathFileTableWidget->item(index.row(), 1)->setText(value);
}
