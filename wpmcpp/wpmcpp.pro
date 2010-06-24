# -------------------------------------------------
# Project created by QtCreator 2010-06-20T15:15:32
# -------------------------------------------------
QT += network \
    xml
TARGET = wpmcpp
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    packageversion.cpp \
    repository.cpp \
    job.cpp \
    downloader.cpp
HEADERS += mainwindow.h \
    packageversion.h \
    repository.h \
    job.h \
    downloader.h
FORMS += mainwindow.ui
INCLUDEPATH += C:\Users\t\libs\quazip-0.2.3\quazip C:\Users\t\libs\zlib-1.2.5
LIBS += C:\Users\t\libs\quazip-0.2.3\quazip\debug\libquazip.a C:\Users\t\libs\zlib-1.2.5\libz.a

