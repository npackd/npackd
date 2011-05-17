# -------------------------------------------------
# Project created by QtCreator 2010-11-21T18:29:49
# -------------------------------------------------
QT += xml
QT -= gui
TARGET = npackdcl
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
RC_FILE = npackdcl.rc
INCLUDEPATH += C:\Users\t\libs\quazip-0.2.3\quazip \
    C:\Users\t\libs\zlib-1.2.5
LIBS += C:\Users\t\libs\quazip-0.2.3\quazip\debug\libquazip.a \
    C:\Users\t\libs\zlib-1.2.5\libz.a \
    C:\Qt\2010.03\mingw\lib\libole32.a \
    C:\Qt\2010.03\mingw\lib\libuuid.a \
    C:\Qt\2010.03\mingw\lib\libwininet.a \
    C:\Qt\2010.03\mingw\lib\libpsapi.a \
    C:\Qt\2010.03\mingw\lib\libversion.a \
    C:\Users\t\projects\windows-package-manager\wpmcpp\libmsi.a
SOURCES += main.cpp \
    ../wpmcpp/repository.cpp \
    ../wpmcpp/version.cpp \
    ../wpmcpp/packageversionfile.cpp \
    ../wpmcpp/package.cpp \
    ../wpmcpp/packageversion.cpp \
    ../wpmcpp/job.cpp \
    ../wpmcpp/installoperation.cpp \
    ../wpmcpp/digraph.cpp \
    ../wpmcpp/dependency.cpp \
    ../wpmcpp/wpmutils.cpp \
    ../wpmcpp/downloader.cpp \
    ../wpmcpp/node.cpp \
    ../wpmcpp/license.cpp \
    ../wpmcpp/windowsregistry.cpp \
    ../wpmcpp/detectfile.cpp \
    app.cpp \
    ../wpmcpp/commandline.cpp
HEADERS += ../wpmcpp/repository.h \
    ../wpmcpp/version.h \
    ../wpmcpp/packageversionfile.h \
    ../wpmcpp/package.h \
    ../wpmcpp/packageversion.h \
    ../wpmcpp/job.h \
    ../wpmcpp/installoperation.h \
    ../wpmcpp/digraph.h \
    ../wpmcpp/dependency.h \
    ../wpmcpp/wpmutils.h \
    ../wpmcpp/msi.h \
    ../wpmcpp/downloader.h \
    ../wpmcpp/node.h \
    ../wpmcpp/license.h \
    ../wpmcpp/windowsregistry.h \
    ../wpmcpp/detectfile.h \
    app.h \
    ../wpmcpp/commandline.h
FORMS += 
