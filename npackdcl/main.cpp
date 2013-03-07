#include <windows.h>
#include <qdebug.h>
#include <qstringlist.h>
#include <qstring.h>

#include "..\wpmcpp\repository.h"
#include "..\wpmcpp\commandline.h"
#include "..\wpmcpp\wpmutils.h"
#include "..\wpmcpp\abstractrepository.h"
#include "..\wpmcpp\dbrepository.h"

#include "app.h"

int main(int argc, char *argv[])
{
#ifndef __MINGW64__
    LoadLibrary(L"exchndl.dll");
#endif

    AbstractRepository::setDefault_(DBRepository::getDefault());

    CoInitializeEx(0, COINIT_MULTITHREADED);

    qRegisterMetaType<JobState>("JobState");

    App app;

    // TODO: handle error
    DBRepository::getDefault()->open();

    return app.process();
}

