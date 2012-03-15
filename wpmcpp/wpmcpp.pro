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
    packageversionfile.cpp \
    version.cpp \
    dependency.cpp \
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
    xmlutils.cpp \
    settingsframe.cpp
HEADERS += mainwindow.h \
    packageversion.h \
    repository.h \
    job.h \
    downloader.h \
    wpmutils.h \
    package.h \
    packageversionfile.h \
    version.h \
    dependency.h \
    msi.h \
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
    taskbar.h \
    xmlutils.h \
    settingsframe.h \
    mstask.h
FORMS += mainwindow.ui \
    packageversionform.ui \
    licenseform.ui \
    progressframe.ui \
    messageframe.ui \
    settingsframe.ui
INCLUDEPATH += ..\\quazip\\quazip ..\\zlib
LIBS += ..\\quazip\\quazip\\release\\libquazip.a \
    ..\\zlib\\libz.a \
    -lole32 \
    -luuid \
    -lwininet \
    -lpsapi \
    -lshell32 \
    -lversion \
    ..\\wpmcpp\\libmsi.a
CONFIG += embed_manifest_exe
RC_FILE = wpmcpp.rc
RESOURCES += wpmcpp.qrc

# these 2 options can be used to add the debugging information to the "release"
# build
# QMAKE_CXXFLAGS_RELEASE += -g
# QMAKE_LFLAGS_RELEASE -= -Wl,-s
