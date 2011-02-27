#ifndef APP_H
#define APP_H

#include <iostream>
#include <iomanip>

#include <QtCore/QCoreApplication>
#include <qdebug.h>
#include <qstringlist.h>
#include <qstring.h>

#include "..\wpmcpp\repository.h"

class App: public QObject
{
    Q_OBJECT
private slots:
    void jobChanged(const JobState& s);
private:
    QStringList params;

    void usage();
    int path();
    int add();
    int remove();
public:
    int process(const QStringList& params);
};

#endif // APP_H
