#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    a.setQuitOnLastWindowClosed(false);
//    w.show();
//    w.hide();

    return a.exec();
}
