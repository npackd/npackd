# -------------------------------------------------
# Project created by QtCreator 2010-06-20T15:15:32
# -------------------------------------------------
QT += xml
TARGET = wpmcpp
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    packageversion.cpp \
    repository.cpp \
    job.cpp \
    downloader.cpp \
    wpmutils.cpp \
    package.cpp \
    progressdialog.cpp \
    packageversionfile.cpp \
    version.cpp \
    settingsdialog.cpp \
    dependency.cpp \
    node.cpp \
    digraph.cpp \
    fileloader.cpp \
    fileloaderitem.cpp \
    installoperation.cpp \
    packageversionform.cpp \
    license.cpp \
    licenseform.cpp \
    windowsregistry.cpp \
    detectfile.cpp \
    uiutils.cpp
HEADERS += mainwindow.h \
    packageversion.h \
    repository.h \
    job.h \
    downloader.h \
    wpmutils.h \
    package.h \
    progressdialog.h \
    packageversionfile.h \
    version.h \
    settingsdialog.h \
    dependency.h \
    msi.h \
    node.h \
    digraph.h \
    fileloader.h \
    fileloaderitem.h \
    installoperation.h \
    packageversionform.h \
    license.h \
    licenseform.h \
    windowsregistry.h \
    detectfile.h \
    uiutils.h
FORMS += mainwindow.ui \
    progressdialog.ui \
    settingsdialog.ui \
    packageversionform.ui \
    licenseform.ui
INCLUDEPATH += C:\Users\t\libs\quazip-0.2.3\quazip \
    C:\Users\t\libs\zlib-1.2.5
LIBS += C:\Users\t\libs\quazip-0.2.3\quazip\debug\libquazip.a \
    C:\Users\t\libs\zlib-1.2.5\libz.a \
    C:\Qt\2010.03\mingw\lib\libole32.a \
    C:\Qt\2010.03\mingw\lib\libuuid.a \
    C:\Qt\2010.03\mingw\lib\libwininet.a \
    C:\Qt\2010.03\mingw\lib\libpsapi.a \
    C:\Qt\2010.03\mingw\lib\libshell32.a \
    C:\Qt\2010.03\mingw\lib\libversion.a \
    C:\Users\t\projects\windows-package-manager\wpmcpp\libmsi.a
CONFIG += embed_manifest_exe
RC_FILE = wpmcpp.rc
RESOURCES += wpmcpp.qrc
