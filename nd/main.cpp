#include <qdebug.h>
#include <qstringlist.h>
#include <qstring.h>

#include "..\wpmcpp\repository.h"
#include "..\wpmcpp\commandline.h"
#include "..\wpmcpp\wpmutils.h"

#include "ndapp.h"

int main(int argc, char *argv[])
{
    CoInitializeEx(0, COINIT_MULTITHREADED);

    qRegisterMetaType<JobState>("JobState");

    App app;

    return app.process();
}

