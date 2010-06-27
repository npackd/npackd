#include <QtGui/QApplication>
#include "mainwindow.h"
#include "qnetworkproxy.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QNetworkProxyFactory::setUseSystemConfiguration(true);
    MainWindow w;
    w.show();
    w.loadRepository(); // TODO: handle errors
    return a.exec();
}
