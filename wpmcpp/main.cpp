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
    //CoInitialize(NULL);
    //WPMUtils::createMSTask();

#ifndef __MINGW64__
    LoadLibrary(L"exchndl.dll");
#endif

    QApplication a(argc, argv);

    qRegisterMetaType<JobState>("JobState");
    qRegisterMetaType<FileLoaderItem>("FileLoaderItem");

#ifndef __MINGW64__
    if (WPMUtils::is64BitWindows()) {
        QMessageBox::critical(0, "Error",
                "The 32 bit version of Npackd requires a 32 bit operating system.\n"
                "Please download the 64 bit version from http://code.google.com/p/windows-package-manager/");
        return 1;
    }
#endif

    MainWindow w;

    w.prepare();
    w.showMaximized();
    return QApplication::exec();
}
