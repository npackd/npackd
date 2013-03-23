#ifndef APP_H
#define APP_H

#include <time.h>

#include <QtCore/QCoreApplication>
#include <qdebug.h>
#include <qstringlist.h>
#include <qstring.h>

#include "..\wpmcpp\repository.h"
#include "..\wpmcpp\commandline.h"
#include "..\wpmcpp\job.h"
#include "..\wpmcpp\clprogress.h"

/**
 * NpackdCL
 */
class App: public QObject
{
    Q_OBJECT
private:
    CommandLine cl;
    CLProgress clp;

    void addNpackdCL();

    void usage();
    int path();
    int add();
    int remove();
    QString addRepo();
    QString removeRepo();
    QString search();
    int list();
    QString info();
    int update();
    int detect();
    QString listRepos();
    QString which();

    bool confirm(const QList<InstallOperation *> ops, QString *title);

    /**
     * @param package full or short package name
     * @param err error message will be stored here
     * @return [ownership:caller] found package or 0. The returned value is
     *     only 0 if the error is not empty
     */
    Package *findOnePackage(const QString &package, QString *err);
public:
    /**
     * Process the command line.
     *
     * @return exit code
     */
    int process();
};

#endif // APP_H
