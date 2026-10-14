#include "mainwindow.h"
#include "theme.h"
#include "updater.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    Updater::startup();
    QApplication a(argc, argv);
    a.setStyle("Fusion");
    Theme::apply(a);
    a.setOrganizationName("OpenKogama");
    a.setApplicationName("OpenKogama Launcher");
    MainWindow w;
    w.show();
    Updater::check(&w);
    return QApplication::exec();
}