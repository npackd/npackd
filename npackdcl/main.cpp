#include <iostream>
#include <iomanip>

#include <QtCore/QCoreApplication>
#include <qdebug.h>
#include <qstringlist.h>
#include <qstring.h>

#include "..\wpmcpp\repository.h"
#include "..\wpmcpp\commandline.h"

#include "app.h"

int main(int argc, char *argv[])
{
    // removes the current directory from the default DLL search order. This
    // helps against viruses.
    // requires 0x0502
    // SetDllDirectory("");

    LoadLibrary(L"exchndl.dll");

    qRegisterMetaType<JobState>("JobState");

    App app;
    return app.process(argc, argv);
}

