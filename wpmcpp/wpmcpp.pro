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
    uiutils.cpp \
    commandline.cpp \
    progressframe.cpp \
    messageframe.cpp \
    minidumper.cpp
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
    uiutils.h \
    commandline.h \
    progressframe.h \
    messageframe.h \
    minidumper.h
FORMS += mainwindow.ui \
    progressdialog.ui \
    settingsdialog.ui \
    packageversionform.ui \
    licenseform.ui \
    progressframe.ui \
    messageframe.ui
INCLUDEPATH += ..\\quazip\\quazip ..\\zlib
MINGWLIBS=C:\\QtSDK-1.1.1\\mingw\\lib
LIBS += ..\\quazip\\quazip\\release\\libquazip.a \
    ..\\zlib\\libz.a \
    $$MINGWLIBS\\libole32.a \
    $$MINGWLIBS\\libuuid.a \
    $$MINGWLIBS\\libwininet.a \
    $$MINGWLIBS\\libpsapi.a \
    $$MINGWLIBS\\libshell32.a \
    $$MINGWLIBS\\libversion.a \
    ..\\wpmcpp\\libmsi.a
CONFIG += embed_manifest_exe
RC_FILE = wpmcpp.rc
RESOURCES += wpmcpp.qrc
QMAKE_CXXFLAGS_RELEASE += -g
QMAKE_LFLAGS_RELEASE -= -Wl,-s
