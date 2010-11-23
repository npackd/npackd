#include <iostream>
#include <iomanip>

#include <QtCore/QCoreApplication>
#include <qdebug.h>
#include <qstringlist.h>
#include <qstring.h>

#include "..\wpmcpp\repository.h"

void usage()
{
    std::cout << "Npackd command line tool" << std::endl;
    std::cout << "Usage: npackdcl find --package=<package> --versions=<versions>" << std::endl;
    std::cout << "or" << std::endl;
    std::cout << "Usage: npackdcl list" << std::endl;
    std::cout << "or" << std::endl;
    std::cout << "Usage: npackdcl info" << std::endl;
}

int main(int argc, char *argv[])
{
    // removes the current directory from the default DLL search order. This
    // helps against viruses.
    // requires 0x0502
    // SetDllDirectory("");

    LoadLibrary(L"exchndl.dll");

    QStringList params;
    for (int i = 0; i < argc; i++) {
        params.append(QString(argv[i]));
    }

    /* debugging
    for (int i = 0; i < params.count(); i++) {
        QString s = params.at(i);
        std::cout << qPrintable(s) << std::endl;
    }
    */

    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = new Job();
    rep->recognize(job);
    delete job;
    rep->addUnknownExistingPackages();

    if (params.count() == 4) {
        QString p2 = params.at(2);
        QString p3 = params.at(3);
        if (params.at(1) == "path" && p2.startsWith("--package=") &&
                p3.startsWith("--versions=")) {
            QString package = p2.right(p2.length() - 10);
            if (!Package::isValidName(package)) {
                std::cout << "Invalid package name: " << qPrintable(package) << std::endl;
                usage();
                r = 1;
            } else {
                QString versions = p3.right(p3.length() - 11);
                // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
                Dependency d;
                d.package = package;
                if (!d.setVersions(versions)) {
                    std::cout << "Cannot parse versions: " << qPrintable(versions) << std::endl;
                    usage();
                    r = 1;
                } else {
                    // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
                    PackageVersion* pv = d.findHighestInstalledMatch();
                    if (pv)
                        std::cout << qPrintable(pv->getDirectory().absolutePath()) << std::endl;
                    else
                        std::cout << "Nothing found" << std::endl;
                }
            }
        } else {
            std::cout << "Wrong arguments" << std::endl;
            usage();
            r = 1;
        }
    } else if (params.count() == 2 && params.at(1) == "list") {
        QList<PackageVersion*> installed = rep->getInstalled();
        for (int i = 0; i < installed.count(); i++) {
            std::cout << qPrintable(installed.at(i)->package) << " " <<
                    qPrintable(installed.at(i)->version.getVersionString()) <<
                    std::endl;
        }
    } else if (params.count() == 2 && params.at(1) == "info") {
        std::cout << "Installation directory: " <<
                qPrintable(rep->getDirectory().absolutePath()) << std::endl;
        QList<PackageVersion*> installed = rep->getInstalled();
        std::cout << "Number of installed packages: " <<
                installed.count() << std::endl;
    } else {
        std::cout << "Wrong arguments" << std::endl;
        usage();
        r = 1;
    }

    return r;
}
