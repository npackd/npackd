#define _WIN32_IE 0x0500

#include <windows.h>
#include <shellapi.h>

#include <QtGui/QApplication>
#include "QMetaType"
#include "mainwindow.h"
#include "qdebug.h"

#include "wpmutils.h"
#include "job.h"
#include "fileloader.h"

int main(int argc, char *argv[])
{
    //CoInitialize(NULL);
    //WPMUtils::createMSTask();

    QApplication a(argc, argv);

    qRegisterMetaType<JobState>("JobState");
    qRegisterMetaType<FileLoaderItem>("FileLoaderItem");

    MainWindow w;

    w.prepare();
    w.showMaximized();
    return QApplication::exec();
}
