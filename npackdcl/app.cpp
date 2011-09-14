#include <limits>
#include "math.h"

#include "app.h"
#include "..\wpmcpp\wpmutils.h"
#include "..\wpmcpp\commandline.h"

void App::jobChanged(const JobState& s)
{
    HANDLE hOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    time_t now = time(0);
    if (!s.completed) {
        if (now - this->lastJobChange != 0) {
            int w = progressPos.dwSize.X - 6;

            SetConsoleCursorPosition(hOutputHandle,
                    progressPos.dwCursorPosition);
            QString txt = s.hint;
            if (txt.length() >= w)
                txt = "..." + txt.right(w - 3);
            if (txt.length() < w)
                txt = txt + QString().fill(' ', w - txt.length());
            txt += QString("%1%").arg(floor(s.progress * 100 + 0.5), 4);
            WPMUtils::outputTextConsole(txt);
        }
    } else {
        QString filled;
        filled.fill(' ', progressPos.dwSize.X - 1);
        SetConsoleCursorPosition(hOutputHandle, progressPos.dwCursorPosition);
        WPMUtils::outputTextConsole(filled);
        SetConsoleCursorPosition(hOutputHandle, progressPos.dwCursorPosition);
    }
}

QString App::testDependsOnItself()
{
    QString err;

    Repository* r = Repository::getDefault();

    QDomDocument doc;
    int errorLine, errorColumn;
    QFile f("npackdcl\\TestDependsOnItself.xml");
    if (!f.open(QIODevice::ReadOnly))
        err = "Cannot open the repository file";

    if (err.isEmpty())
        doc.setContent(&f, false, &err, &errorLine, &errorColumn);

    if (err.isEmpty()) {
        Job* job = new Job();
        r->loadOne(&doc, job);
        err = job->getErrorMessage();
        delete job;

        r->readRegistryDatabase();
    }

    PackageVersion* pv = r->findPackageVersion(
            "com.googlecode.windows-package-manager.Test",
            Version(1, 0));

    if (err.isEmpty()) {
        if (pv->installed()) {
            Job* job = new Job();
            pv->uninstall(job);
            err = job->getErrorMessage();
            delete job;
        }
    }

    if (err.isEmpty()) {
        QList<PackageVersion*> installed;
        QList<PackageVersion*> avoid;
        QList<InstallOperation*> ops;
        err = pv->planInstallation(installed, ops, avoid);

        if (err.isEmpty()) {
            err = "Packages cannot depend on itself";
        } else {
            err = "";
        }
    }

    return err;
}

int App::unitTests()
{
    WPMUtils::outputTextConsole("Starting internal tests\n");

    WPMUtils::outputTextConsole("testDependsOnItself\n");
    QString err = testDependsOnItself();
    if (err.isEmpty())
        WPMUtils::outputTextConsole("Internal tests were successful\n");
    else
        WPMUtils::outputTextConsole("Internal tests failed: " + err + "\n");

    return 0;
}

int App::process(int argc, char *argv[])
{
    cl.add("package", 'p', "internal package name (e.g. com.example.Editor)",
            "package", false);
    cl.add("versions", 'r', "versions range (e.g. [1.5,2))",
            "range", false);
    cl.add("version", 'v', "version number (e.g. 1.5.12)",
            "version", false);
    cl.add("url", 'u', "repository URL (e.g. https://www.example.com/Rep.xml)",
            "repository", false);

    QString err = cl.parse(argc, argv);
    if (!err.isEmpty()) {
        WPMUtils::outputTextConsole("Error: " + err + "\n");
        return 1;
    }
    // cl.dump();

    int r = 0;

    QStringList fr = cl.getFreeArguments();

    if (fr.count() == 0) {
        std::cerr << "Missing command. Try npackdcl help" << std::endl;
        r = 1;
    } else if (fr.count() > 1) {
        std::cerr << "Unexpected argument: " << qPrintable(fr.at(1)) << std::endl;
        r = 1;
    } else if (fr.at(0) == "help") {
        usage();
    } else if (fr.at(0) == "path") {
        r = path();
    } else if (fr.at(0) == "remove") {
        r = remove();
    } else if (fr.at(0) == "add") {
        r = add();
    } else if (fr.at(0) == "add-repo") {
        r = addRepo();
    } else if (fr.at(0) == "remove-repo") {
        r = removeRepo();
    } else if (fr.at(0) == "list") {
        r = list();
    } else if (fr.at(0) == "unit-tests") {
        r = unitTests();
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
        std::cerr << "Wrong command: " << qPrintable(fr.at(0)) << std::endl;
        r = 1;
    }

    return r;
}

void App::addNpackdCL()
{
    Repository* r = Repository::getDefault();
    PackageVersion* pv = r->findOrCreatePackageVersion(
            "com.googlecode.windows-package-manager.NpackdCL",
            Version(WPMUtils::NPACKD_VERSION));
    if (!pv->installed()) {
        pv->setPath(WPMUtils::getExeDir());
        pv->setExternal(true);
        r->updateNpackdCLEnvVar();
    }
}

Job* App::createJob()
{
    HANDLE hOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hOutputHandle, &progressPos);
    if (progressPos.dwCursorPosition.Y >= progressPos.dwSize.Y - 1) {
        WPMUtils::outputTextConsole("\n");
        progressPos.dwCursorPosition.Y--;
    }

    Job* job = new Job();
    connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(jobChanged(const JobState&)));

    // -1 so that we do not have the initial 1 second delay
    this->lastJobChange = time(0) - 1;

    return job;
}

void App::usage()
{
    const char* lines[] = {
        "Npackd command line tool",
        "Usage:",
        "    npackdcl help",
        "        prints this help",
        "    npackdcl add --package=<package> [--version=<version>]",
        "        installs a package",
        "    npackdcl remove --package=<package> --version=<version>",
        "        removes a package",
        "    npackdcl list",
        "        lists all installed packages",
        "    npackdcl path --package=<package> [--versions=<versions>]",
        "        searches for an installed package and prints its location",
        "    npackdcl add-repo --url=<repository>",
        "        appends a repository to the list",
        "    npackdcl remove-repo --url=<repository>",
        "        removes a repository from the list",
        "Options:",
        "    -p, --package package name",
        "    -r, --versions version range (e.g. [1.1,1.2) )",
        "    -v, --version version (e.g. 1.2.47)",
        "    -u, --url repository URL (e.g. https://www.example.com/Rep.xml)",
        "",
        "You can use short package names in 'add' and 'remove' operations.",
        "Example: App instead of com.example.App",
        "The process exits with the code unequal to 0 if an error occcures."
        /*
        "Usage: npackdcl list"
        "or" << std::endl;
        "Usage: npackdcl info"*/
    };
    for (int i = 0; i < (int) (sizeof(lines) / sizeof(lines[0])); i++) {
        WPMUtils::outputTextConsole(QString(lines[i]) + "\n");
    }
}

int App::addRepo()
{
    int r = 0;

    QString url = cl.get("url");

    if (r == 0) {
        if (url.isNull()) {
            std::cerr << "Missing option: --url" << std::endl;
            r = 1;
        }
    }

    QUrl* url_ = 0;
    if (r == 0) {
        url_ = new QUrl();
        url_->setUrl(url, QUrl::StrictMode);
        if (!url_->isValid()) {
            std::cerr << "Invalid URL: " << qPrintable(url) << std::endl;
            r = 1;
        }
    }

    if (r == 0) {
        Repository* rep = Repository::getDefault();
        QList<QUrl*> urls = rep->getRepositoryURLs();
        int found = -1;
        for (int i = 0; i < urls.size(); i++) {
            if (urls.at(i)->toString() == url_->toString()) {
                found = i;
                break;
            }
        }
        if (found >= 0) {
            WPMUtils::outputTextConsole(
                    "This repository is already registered: " + url + "\n");
        } else {
            urls.append(url_);
            url_ = 0;
            rep->setRepositoryURLs(urls);
        }
        qDeleteAll(urls);
    }

    delete url_;

    return r;
}

int App::list()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = createJob();
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    if (r == 0) {
        Repository* rep = Repository::getDefault();
        for (int i = 0; i < rep->packageVersions.count(); i++) {
            PackageVersion* pv = rep->packageVersions.at(i);
            if (pv->installed()) {
                WPMUtils::outputTextConsole(pv->toString() + "\n");
                WPMUtils::outputTextConsole("    Path: " + pv->getPath() + "\n");
                WPMUtils::outputTextConsole("    Internal name: "  + pv->package + "\n");
            }
        }
    }

    return r;
}

int App::removeRepo()
{
    int r = 0;

    QString url = cl.get("url");

    if (r == 0) {
        if (url.isNull()) {
            std::cerr << "Missing option: --url" << std::endl;
            r = 1;
        }
    }

    QUrl* url_ = 0;
    if (r == 0) {
        url_ = new QUrl();
        url_->setUrl(url, QUrl::StrictMode);
        if (!url_->isValid()) {
            std::cerr << "Invalid URL: " << qPrintable(url) << std::endl;
            r = 1;
        }
    }

    if (r == 0) {
        Repository* rep = Repository::getDefault();
        QList<QUrl*> urls = rep->getRepositoryURLs();
        int found = -1;
        for (int i = 0; i < urls.size(); i++) {
            if (urls.at(i)->toString() == url_->toString()) {
                found = i;
                break;
            }
        }
        if (found < 0) {
            WPMUtils::outputTextConsole("The repository was not in the list: " +
                    url + "\n");
        } else {
            delete urls.takeAt(found);
            rep->setRepositoryURLs(urls);
        }
        qDeleteAll(urls);
    }

    delete url_;

    return r;
}

int App::path()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = new Job();
    rep->refresh(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString versions = cl.get("versions");

    if (r == 0) {
        if (package.isNull()) {
            std::cerr << "Missing option: --package" << std::endl;
            r = 1;
        }
    }

    if (r == 0) {
        if (!Package::isValidName(package)) {
            std::cerr << "Invalid package name: " << qPrintable(package) << std::endl;
            r = 1;
        }
    }

    if (r == 0) {
        // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
        Dependency d;
        d.package = package;
        if (versions.isNull()) {
            d.min.setVersion(0, 0);
            d.max.setVersion(std::numeric_limits<int>::max(), 0);
        } else {
            if (!d.setVersions(versions)) {
                std::cerr << "Cannot parse versions: " << qPrintable(versions) << std::endl;
                r = 1;
            }
        }

        if (r == 0) {
            // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
            PackageVersion* pv = d.findHighestInstalledMatch();
            if (pv) {
                QString p = pv->getPath();
                p.replace('/', '\\');
                WPMUtils::outputTextConsole(p + "\n");
            }
        }
    }

    return r;
}

int App::add()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = createJob();
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString version = cl.get("version");

    if (r == 0) {
        if (package.isNull()) {
            std::cerr << "Missing option: --package" << std::endl;
            r = 1;
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
            QList<Package*> packages = rep->findPackages(package);
            if (packages.count() == 0) {
                std::cerr << "Unknown package: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }
            if (packages.count() > 1) {
                std::cerr << "Ambiguous package name" << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
            PackageVersion* pv;
            if (version.isNull()) {
                pv = rep->findNewestInstallablePackageVersion(
                        packages.at(0)->name);
            } else {
                Version v;
                if (!v.setVersion(version)) {
                    std::cerr << "Cannot parse version: " << qPrintable(version) << std::endl;
                    r = 1;
                    break;
                }
                pv = rep->findPackageVersion(packages.at(0)->name, version);
            }

            // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
            if (!pv) {
                std::cerr << "Package version not found" << std::endl;
                r = 1;
                break;
            }
            if (pv->installed()) {
                std::cerr << "Package is already installed in " <<
                        qPrintable(pv->getPath()) << std::endl;
                r = 0;
                break;
            }

            QList<InstallOperation*> ops;
            QList<PackageVersion*> installed =
                    Repository::getDefault()->getInstalled();
            QList<PackageVersion*> avoid;
            QString err = pv->planInstallation(installed, ops, avoid);
            if (!err.isEmpty()) {
                std::cerr << qPrintable(err) << std::endl;
                r = 1;
                break;
            }

            Job* ijob = createJob();
            rep->process(ijob, ops);
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
    Job* job = createJob();
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString version = cl.get("version");

    if (r == 0) {
        if (package.isNull()) {
            std::cerr << "Missing option: --package" << std::endl;
            r = 1;
        }
    }

    if (r == 0) {
        if (version.isNull()) {
            std::cerr << "Missing option: --version" << std::endl;
            r = 1;
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
            QList<Package*> packages = rep->findPackages(package);
            if (packages.count() == 0) {
                std::cerr << "Unknown package: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }
            if (packages.count() > 1) {
                std::cerr << "Ambiguous package name" << std::endl;
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
            PackageVersion* pv = rep->findPackageVersion(
                    packages.at(0)->name, version);
            if (!pv) {
                std::cerr << "Package not found" << std::endl;
                r = 1;
                break;
            }

            if (!pv->installed()) {
                std::cerr << "Package is not installed" << std::endl;
                r = 0;
                break;
            }

            if (pv->isExternal()) {
                std::cerr << "Externally installed packages cannot be removed" << std::endl;
                r = 1;
                break;
            }

            QList<InstallOperation*> ops;
            QList<PackageVersion*> installed =
                    Repository::getDefault()->getInstalled();
            QString err = pv->planUninstallation(installed, ops);
            if (!err.isEmpty()) {
                std::cerr << qPrintable(err) << std::endl;
                r = 1;
                break;
            }

            Job* job = createJob();
            rep->process(job, ops);
            if (!job->getErrorMessage().isEmpty()) {
                std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
                r = 1;
            }
            delete job;
        } while (false);
    }

    return r;
}
