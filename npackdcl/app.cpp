#include "app.h"

void App::jobChanged(const JobState& s)
{
    std::cout << qPrintable(s.hint) << std::endl;
}

int App::process(const QStringList &params)
{
    this->params = params;

    int r = 0;

    if (params.count() <= 1) {
        std::cerr << "Missing arguments. Try npackdcl.exe help" << std::endl;
        r = 1;
    } else if (params.at(1) == "help") {
        usage();
    } else if (params.at(1) == "path") {
        r = path();
    } else if (params.at(1) == "remove") {
        r = remove();
    } else if (params.at(1) == "add") {
        r = add();
    /*} else if (params.count() == 2 && params.at(1) == "list") {
        QList<PackageVersion*> installed = rep->packageVersions; // getInstalled();
        for (int i = 0; i < installed.count(); i++) {
            std::cout << qPrintable(installed.at(i)->getPackageTitle()) << " " <<
                    qPrintable(installed.at(i)->version.getVersionString()) <<
                    " " << qPrintable(installed.at(i)->getPath()) <<
                    std::endl;
        }
    } else if (params.count() == 2 && params.at(1) == "info") {
        / *std::cout << "Installation directory: " <<
                qPrintable(rep->getDirectory().absolutePath()) << std::endl;
        QList<PackageVersion*> installed = rep->getInstalled();
        std::cout << "Number of installed packages: " <<
                installed.count() << std::endl;*/
    } else {
        std::cerr << "Wrong arguments" << std::endl;
        usage();
        r = 1;
    }

    return r;
}

void App::usage()
{
    std::cout << "Npackd command line tool" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "    npackdcl help" << std::endl;
    std::cout << "    or" << std::endl;
    std::cout << "    npackdcl path --package=<package> --versions=<versions>" << std::endl;
    std::cout << "    or" << std::endl;
    std::cout << "    npackdcl add --package=<package> --version=<version>" << std::endl;
    std::cout << "    or" << std::endl;
    std::cout << "    npackdcl remove --package=<package> --version=<version>" << std::endl;
    std::cout << "The process exits with the code unequal 0 if an error occcures." << std::endl;
    /*
    std::cout << "Usage: npackdcl list" << std::endl;
    std::cout << "or" << std::endl;
    std::cout << "Usage: npackdcl info" << std::endl;*/
}

int App::path()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = new Job();
    /*connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(jobChanged(const JobState&)),
            Qt::dConnection); todo */
    rep->refresh(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    QString package;
    QString versions;
    if (r == 0) {
        for (int i = 2; i < params.count(); i++) {
            QString p = params.at(i);
            if (p.startsWith("--package=")) {
                package = p.right(p.length() - 10);
            } else if (p.startsWith("--versions=")) {
                versions = p.right(p.length() - 11);
            } else {
                std::cerr << "Unknown argument: " << qPrintable(p) << std::endl;
                r = 1;
                break;
            }
        }
    }

    if (r == 0) {
        if (!Package::isValidName(package)) {
            std::cerr << "Invalid package name: " << qPrintable(package) << std::endl;
            usage();
            r = 1;
        } else {
            // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
            Dependency d;
            d.package = package;
            if (!d.setVersions(versions)) {
                std::cerr << "Cannot parse versions: " << qPrintable(versions) << std::endl;
                usage();
                r = 1;
            } else {
                // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
                PackageVersion* pv = d.findHighestInstalledMatch();
                if (pv) {
                    std::cout << qPrintable(pv->getPath()) << std::endl;
                } else {
                    std::cout << "nothing" << qPrintable(d.toString()) << std::endl;
                }
            }
        }
    }

    return r;
}

int App::add()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = new Job();
    connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(jobChanged(const JobState&)));
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    QString package;
    QString version;
    if (r == 0) {
        for (int i = 2; i < params.count(); i++) {
            QString p = params.at(i);
            if (p.startsWith("--package=")) {
                package = p.right(p.length() - 10);
            } else if (p.startsWith("--version=")) {
                version = p.right(p.length() - 10);
            } else {
                std::cerr << "Unknown argument: " << qPrintable(p) << std::endl;
                r = 1;
                break;
            }
        }
    }

    if (r == 0) {
        do {
            if (!Package::isValidName(package)) {
                std::cerr << "Invalid package name: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }

            Repository* rep = Repository::getDefault();
            if (!rep->findPackage(package)) {
                std::cerr << "Unknown package: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
            Version v;
            if (!v.setVersion(version)) {
                std::cerr << "Cannot parse version: " << qPrintable(version) << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
            PackageVersion* pv = rep->findPackageVersion(package, version);
            if (!pv) {
                std::cerr << "Package version not found" << std::endl;
                r = 1;
                break;
            }
            if (pv->installed()) {
                std::cerr << "Package is already installed" << std::endl;
                r = 1;
                break;
            }

            std::cout << "installing..." << std::endl;

            Job* ijob = new Job();
            connect(ijob, SIGNAL(changed(const JobState&)), this,
                    SLOT(jobChanged(const JobState&)));
            QString where = pv->getPreferredInstallationDirectory();
            pv->install(ijob, where);
            if (!ijob->getErrorMessage().isEmpty()) {
                std::cerr << qPrintable(ijob->getErrorMessage()) << std::endl;
                r = 1;
            }
            delete ijob;
        } while (false);
    }

    return r;
}

int App::remove()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = new Job();
    connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(jobChanged(const JobState&)));
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    QString package;
    QString version;
    if (r == 0) {
        for (int i = 2; i < params.count(); i++) {
            QString p = params.at(i);
            if (p.startsWith("--package=")) {
                package = p.right(p.length() - 10);
            } else if (p.startsWith("--version=")) {
                version = p.right(p.length() - 10);
            } else {
                std::cerr << "Unknown argument: " << qPrintable(p) << std::endl;
                r = 1;
                break;
            }
        }
    }

    if (r == 0) {
        do {
            if (!Package::isValidName(package)) {
                std::cerr << "Invalid package name: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }

            Repository* rep = Repository::getDefault();
            if (!rep->findPackage(package)) {
                std::cerr << "Unknown package: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
            Version v;
            if (!v.setVersion(version)) {
                std::cerr << "Cannot parse version: " << qPrintable(version) << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
            PackageVersion* pv = rep->findPackageVersion(package, version);
            if (!pv) {
                std::cerr << "Package not found" << std::endl;
                r = 1;
                break;

            }

            if (!pv->installed()) {
                std::cerr << "Package is not installed" << std::endl;
                r = 1;
                break;
            }

            if (pv->isExternal()) {
                std::cerr << "Externally installed packages cannot be removed" << std::endl;
                r = 1;
            }

            QList<InstallOperation*> ops;
            QList<PackageVersion*> installed =
                    Repository::getDefault()->getInstalled();
            QString err = pv->planInstallation(installed, ops);
            if (!err.isEmpty()) {
                std::cerr << qPrintable(err) << std::endl;
                r = 1;
                break;
            }

            Job* job = new Job();
            rep->process(job, ops);
            delete job;
        } while (false);
    }

    return r;
}
