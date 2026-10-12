#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleHints>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");
    a.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    QPalette palette = a.palette();
    palette.setColor(QPalette::Highlight, QColor(0x3a, 0x7a, 0x80));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Accent, QColor(0x3a, 0x7a, 0x80));
    a.setPalette(palette);
    QFile style(":/launcher.qss");
    if (style.open(QIODevice::ReadOnly))
        a.setStyleSheet(QString::fromUtf8(style.readAll()));
    a.setOrganizationName("OpenKogama");
    a.setApplicationName("OpenKogama Launcher");
    MainWindow w;
    w.show();
    return QApplication::exec();
}