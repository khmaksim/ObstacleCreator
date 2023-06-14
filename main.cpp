#include "maindialog.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Aviacominfo");
    QCoreApplication::setOrganizationDomain("aviacominfo.com");
    QCoreApplication::setApplicationName("Obstacle Creator");

    QTranslator translator;
    if (translator.load(QLocale(), QLatin1String("obstaclecreator"), QLatin1String("_"), QLatin1String(":/")))
        a.installTranslator(&translator);

    MainDialog w;
    w.show();

    return a.exec();
}
