#include <iostream>
#include <iomanip>

#include <qdebug.h>
#include <qstringlist.h>
#include <qstring.h>

#include "..\wpmcpp\repository.h"
#include "..\wpmcpp\commandline.h"
#include "..\wpmcpp\wpmutils.h"
#include "..\wpmcpp\minidumper.h"

#include "app.h"

MiniDumper md("NpackdCL");

int main(int argc, char *argv[])
{
    // removes the current directory from the default DLL search order. This
    // helps against viruses.
    // requires 0x0502
    // SetDllDirectory("");

    //*(char*) 0 = 0;

    CoInitializeEx(0, COINIT_MULTITHREADED);

    qRegisterMetaType<JobState>("JobState");

    App app;

    return app.process(argc, argv);
}

