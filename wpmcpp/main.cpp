#define _WIN32_IE 0x0500

#include <windows.h>
#include <shellapi.h>

#include <QtGui/QApplication>
#include "QMetaType"
#include "mainwindow.h"
#include "qdebug.h"

#include "repository.h"
#include "wpmutils.h"
#include "job.h"
#include "fileloader.h"

int main(int argc, char *argv[])
{
    /* test
    QString fn("C:\\Program Files (x86)\\wpm\\no.itefix.CWRsyncServer-4.0.4\\Bin\\PrepUploadDir.exe");
    QString p("C:\\Program Files (x86)\\WPM\\no.itefix.CWRsyncServer-4.0.4");
    qDebug() << WPMUtils::isUnder(fn, p);
    */


    // removes the current directory from the default DLL search order. This
    // helps against viruses.
    // requires 0x0502
    // SetDllDirectory("");

    QApplication a(argc, argv);

    qRegisterMetaType<JobState>("JobState");
    qRegisterMetaType<FileLoaderItem>("FileLoaderItem");

    MainWindow w;

    w.prepare();
    w.showMaximized();
    return QApplication::exec();
}
