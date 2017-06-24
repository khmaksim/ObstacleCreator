#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
        Q_OBJECT

    public:
        explicit SettingsDialog(QWidget *parent = 0);
        ~SettingsDialog();

    private:
        Ui::SettingsDialog *ui;

        void readSettings();
        void writeSettings();
        void setDefaultValue();

    private slots:
        void showSelectPath();
};

#endif // SETTINGSDIALOG_H
