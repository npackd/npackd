#include <iostream>
#include <iomanip>

#include <QtCore/QCoreApplication>
#include <qdebug.h>
#include <qstringlist.h>
#include <qstring.h>

#include "..\wpmcpp\repository.h"

#include "app.h"

int main(int argc, char *argv[])
{
    // removes the current directory from the default DLL search order. This
    // helps against viruses.
    // requires 0x0502
    // SetDllDirectory("");

    LoadLibrary(L"exchndl.dll");

    QStringList params;
    for (int i = 0; i < argc; i++) {
        params.append(QString(argv[i]));
    }

    /* debugging
    std::cout << params.count() << std::endl;
    for (int i = 0; i < params.count(); i++) {
        QString s = params.at(i);
        std::cout << qPrintable(s) << std::endl;
    }
    */

    qRegisterMetaType<JobState>("JobState");

    App app;
    return app.process(params);
}

