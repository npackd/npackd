#ifndef APP_H
#define APP_H

#include <iostream>
#include <iomanip>
#include <time.h>

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
    CONSOLE_SCREEN_BUFFER_INFO progressPos;
    QStringList params;
    time_t lastJobChange;

    Job* createJob();
    void usage();
    int path();
    int add();
    int remove();
public:
    /**
     * Process the command line.
     *
     * @param params command line parameters
     * @return exit code
     */
    int process(const QStringList& params);
};

#endif // APP_H
