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
            this->lastJobChange = now;
            if (!WPMUtils::isOutputRedirected(true)) {
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
            } else {
                WPMUtils::outputTextConsole(s.hint + "\n");
            }
        }
    } else {
        if (!WPMUtils::isOutputRedirected(true)) {
            QString filled;
            filled.fill(' ', progressPos.dwSize.X - 1);
            SetConsoleCursorPosition(hOutputHandle, progressPos.dwCursorPosition);
            WPMUtils::outputTextConsole(filled);
            SetConsoleCursorPosition(hOutputHandle, progressPos.dwCursorPosition);
        }
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

int App::process()
{
    cl.add("package", 'p',
            "internal package name (e.g. com.example.Editor or just Editor)",
            "package", false);
    cl.add("versions", 'r', "versions range (e.g. [1.5,2))",
            "range", false);
    cl.add("version", 'v', "version number (e.g. 1.5.12)",
            "version", false);
    cl.add("url", 'u', "repository URL (e.g. https://www.example.com/Rep.xml)",
            "repository", false);
    cl.add("status", 's', "filters package versions by status",
            "status", false);
    cl.add("bare-format", 'b', "bare format (no heading or summary)",
            "", false);

    QString err = cl.parse();
    if (!err.isEmpty()) {
        WPMUtils::outputTextConsole("Error: " + err + "\n");
        return 1;
    }
    // cl.dump();

    int r = 0;

    QStringList fr = cl.getFreeArguments();

    if (fr.count() == 0) {
        WPMUtils::outputTextConsole("Missing command. Try npackdcl help\n", false);
        r = 1;
    } else if (fr.count() > 1) {
        WPMUtils::outputTextConsole("Unexpected argument: " + fr.at(1) + "\n", false);
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
    } else if (fr.at(0) == "info") {
        r = info();
    } else if (fr.at(0) == "update") {
        r = update();
    } else {
        WPMUtils::outputTextConsole("Wrong command: " + fr.at(0) + "\n", false);
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
        "    npackdcl update --package=<package>",
        "        updates a package by uninstalling the currently installed",
        "        and installing the newest version",
        "    npackdcl list [--status=installed | all] [--bare-format]",
        "        lists packages sorted by package name and version.",
        "        Only installed package versions are shown by default.",
        "    npackdcl info --package=<package> --version=<version>",
        "        shows information about the specified package version",
        "    npackdcl path --package=<package> [--versions=<versions>]",
        "        searches for an installed package and prints its location",
        "    npackdcl add-repo --url=<repository>",
        "        appends a repository to the list",
        "    npackdcl remove-repo --url=<repository>",
        "        removes a repository from the list",
        "Options:",
    };
    for (int i = 0; i < (int) (sizeof(lines) / sizeof(lines[0])); i++) {
        WPMUtils::outputTextConsole(QString(lines[i]) + "\n");
    }
    this->cl.printOptions();
    const char* lines2[] = {
        "",
        "You can use short package names in 'add', 'remove' and 'update' operations.",
        "Example: App instead of com.example.App",
        "The process exits with the code unequal to 0 if an error occures.",
        "If the output is redirected, the texts will be encoded as UTF-8.",
    };
    for (int i = 0; i < (int) (sizeof(lines2) / sizeof(lines2[0])); i++) {
        WPMUtils::outputTextConsole(QString(lines2[i]) + "\n");
    }
}

int App::addRepo()
{
    int r = 0;

    QString url = cl.get("url");

    if (r == 0) {
        if (url.isNull()) {
            WPMUtils::outputTextConsole("Missing option: --url\n", false);
            r = 1;
        }
    }

    QUrl* url_ = 0;
    if (r == 0) {
        url_ = new QUrl();
        url_->setUrl(url, QUrl::StrictMode);
        if (!url_->isValid()) {
            WPMUtils::outputTextConsole("Invalid URL: " + url, false);
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
            WPMUtils::outputTextConsole("The repository was added successfully\n");
        }
        qDeleteAll(urls);
    }

    delete url_;

    return r;
}

bool packageVersionLessThan(const PackageVersion* pv1, const PackageVersion* pv2)
{
    if (pv1->package == pv2->package)
        return pv1->version.compare(pv2->version) < 0;
    else {
        QString pt1 = pv1->getPackageTitle();
        QString pt2 = pv2->getPackageTitle();
        return pt1.toLower() < pt2.toLower();
    }
}

int App::list()
{
    bool bare = cl.isPresent("bare-format");

    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job;
    if (bare)
        job = new Job();
    else
        job = createJob();
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        WPMUtils::outputTextConsole(job->getErrorMessage() + "\n", false);
        r = 1;
    }
    delete job;

    bool onlyInstalled = true;
    if (r == 0) {
        QString status = cl.get("status");
        if (!status.isNull()) {
            if (status == "all")
                onlyInstalled = false;
            else if (status == "installed")
                onlyInstalled = true;
            else{
                WPMUtils::outputTextConsole("Wrong status: " + status + "\n", false);
                r = 1;
            }
        }
    }

    if (r == 0) {
        Repository* rep = Repository::getDefault();
        QList<PackageVersion*> list;
        for (int i = 0; i < rep->packageVersions.count(); i++) {
            PackageVersion* pv = rep->packageVersions.at(i);
            if (!onlyInstalled || pv->installed())
                list.append(pv);
        }
        qSort(list.begin(), list.end(), packageVersionLessThan);

        if (!bare)
            WPMUtils::outputTextConsole(QString("%1 package versions found:\n\n").
                    arg(list.count()));

        for (int i = 0; i < list.count(); i++) {
            PackageVersion* pv = list.at(i);
            if (!bare)
                WPMUtils::outputTextConsole(pv->toString() +
                        " (" + pv->package + ")\n");
            else
                WPMUtils::outputTextConsole(pv->package + " " +
                        pv->version.getVersionString() + " " +
                        pv->getPackageTitle() + "\n");
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
            WPMUtils::outputTextConsole("Missing option: --url\n", false);
            r = 1;
        }
    }

    QUrl* url_ = 0;
    if (r == 0) {
        url_ = new QUrl();
        url_->setUrl(url, QUrl::StrictMode);
        if (!url_->isValid()) {
            WPMUtils::outputTextConsole("Invalid URL: " + url + "\n", false);
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
            WPMUtils::outputTextConsole("The repository was removed successfully\n");
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
        WPMUtils::outputTextConsole(job->getErrorMessage() + "\n", false);
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString versions = cl.get("versions");

    if (r == 0) {
        if (package.isNull()) {
            WPMUtils::outputTextConsole("Missing option: --package\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        if (!Package::isValidName(package)) {
            WPMUtils::outputTextConsole("Invalid package name: " + package + "\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        // debug: WPMUtils::outputTextConsole <<  package) << " " << versions);
        Dependency d;
        d.package = package;
        if (versions.isNull()) {
            d.min.setVersion(0, 0);
            d.max.setVersion(std::numeric_limits<int>::max(), 0);
        } else {
            if (!d.setVersions(versions)) {
                WPMUtils::outputTextConsole("Cannot parse versions: " +
                        versions + "\n", false);
                r = 1;
            }
        }

        if (r == 0) {
            // debug: WPMUtils::outputTextConsole << "Versions: " << d.toString()) << std::endl;
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

int App::update()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = createJob();
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        WPMUtils::outputTextConsole(job->getErrorMessage() + "\n", false);
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");

    if (r == 0) {
        if (package.isNull()) {
            WPMUtils::outputTextConsole("Missing option: --package\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        if (!Package::isValidName(package)) {
            WPMUtils::outputTextConsole("Invalid package name: " +
                    package + "\n", false);
            r = 1;
        }
    }

    QList<Package*> packages;

    if (r == 0) {
        packages = rep->findPackages(package);
        if (packages.count() == 0) {
            WPMUtils::outputTextConsole("Unknown package: " +
                    package + "\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        if (packages.count() > 1) {
            WPMUtils::outputTextConsole("Ambiguous package name\n", false);
            r = 1;
        }
    }

    PackageVersion* newesti = 0;
    if (r == 0) {
        // debug: WPMUtils::outputTextConsole <<  package) << " " << versions);
        newesti = rep->findNewestInstalledPackageVersion(
                packages.at(0)->name);
        if (newesti == 0) {
            WPMUtils::outputTextConsole("Package is not installed\n", false);
            r = 1;
        }
    }

    PackageVersion* newest = 0;
    if (r == 0) {
        newest = rep->findNewestInstallablePackageVersion(
            packages.at(0)->name);
        if (newesti == 0) {
            WPMUtils::outputTextConsole("No installable versions found\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        if (newest->version.compare(newesti->version) <= 0) {
            WPMUtils::outputTextConsole("The package is already up-to-date\n", true);
        } else {
            QList<InstallOperation*> ops;
            QString err = rep->planUpdates(packages, ops);

            if (err.isEmpty()) {
                InstallOperation::simplify(ops);
                Job* ijob = createJob();
                rep->process(ijob, ops);
                if (!ijob->getErrorMessage().isEmpty()) {
                    WPMUtils::outputTextConsole(ijob->getErrorMessage() + "\n",
                            false);
                    r = 1;
                } else {
                    WPMUtils::outputTextConsole("The package was updated successfully\n");
                }
                delete ijob;
            } else {
                WPMUtils::outputTextConsole(err + "\n", false);
                r = 1;
            }

            qDeleteAll(ops);
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
        WPMUtils::outputTextConsole(job->getErrorMessage() + "\n", false);
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString version = cl.get("version");

    if (r == 0) {
        if (package.isNull()) {
            WPMUtils::outputTextConsole("Missing option: --package\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        do {
            if (!Package::isValidName(package)) {
                WPMUtils::outputTextConsole("Invalid package name: " +
                        package + "\n", false);
                r = 1;
                break;
            }

            Repository* rep = Repository::getDefault();
            QList<Package*> packages = rep->findPackages(package);
            if (packages.count() == 0) {
                WPMUtils::outputTextConsole("Unknown package: " +
                        package + "\n", false);
                r = 1;
                break;
            }
            if (packages.count() > 1) {
                WPMUtils::outputTextConsole("Ambiguous package name\n", false);
                r = 1;
                break;
            }

            // debug: WPMUtils::outputTextConsole <<  package) << " " << versions);
            PackageVersion* pv;
            if (version.isNull()) {
                pv = rep->findNewestInstallablePackageVersion(
                        packages.at(0)->name);
            } else {
                Version v;
                if (!v.setVersion(version)) {
                    WPMUtils::outputTextConsole("Cannot parse version: " +
                            version + "\n", false);
                    r = 1;
                    break;
                }
                pv = rep->findPackageVersion(packages.at(0)->name, version);
            }

            // debug: WPMUtils::outputTextConsole << "Versions: " << d.toString()) << std::endl;
            if (!pv) {
                WPMUtils::outputTextConsole("Package version not found\n", false);
                r = 1;
                break;
            }
            if (pv->installed()) {
                WPMUtils::outputTextConsole("Package is already installed in " +
                        pv->getPath() + "\n", false);
                r = 0;
                break;
            }

            QList<InstallOperation*> ops;
            QList<PackageVersion*> installed =
                    Repository::getDefault()->getInstalled();
            QList<PackageVersion*> avoid;
            QString err = pv->planInstallation(installed, ops, avoid);
            if (!err.isEmpty()) {
                WPMUtils::outputTextConsole(err + "\n", false);
                r = 1;
                break;
            }

            Job* ijob = createJob();
            rep->process(ijob, ops);
            if (!ijob->getErrorMessage().isEmpty()) {
                WPMUtils::outputTextConsole(ijob->getErrorMessage() + "\n",
                        false);
                r = 1;
            } else {
                WPMUtils::outputTextConsole("The package was installed successfully\n");
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
        WPMUtils::outputTextConsole(job->getErrorMessage() + "\n", false);
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString version = cl.get("version");

    if (r == 0) {
        if (package.isNull()) {
            WPMUtils::outputTextConsole("Missing option: --package\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        if (version.isNull()) {
            WPMUtils::outputTextConsole("Missing option: --version\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        do {
            if (!Package::isValidName(package)) {
                WPMUtils::outputTextConsole("Invalid package name: " +
                        package + "\n", false);
                r = 1;
                break;
            }

            Repository* rep = Repository::getDefault();
            QList<Package*> packages = rep->findPackages(package);
            if (packages.count() == 0) {
                WPMUtils::outputTextConsole("Unknown package: " +
                        package + "\n", false);
                r = 1;
                break;
            }
            if (packages.count() > 1) {
                WPMUtils::outputTextConsole("Ambiguous package name\n", false);
                r = 1;
                break;
            }

            // debug: WPMUtils::outputTextConsole <<  package) << " " << versions);
            Version v;
            if (!v.setVersion(version)) {
                WPMUtils::outputTextConsole("Cannot parse version: " +
                        version + "\n", false);
                r = 1;
                break;
            }

            // debug: WPMUtils::outputTextConsole << "Versions: " << d.toString()) << std::endl;
            PackageVersion* pv = rep->findPackageVersion(
                    packages.at(0)->name, version);
            if (!pv) {
                WPMUtils::outputTextConsole("Package not found\n", false);
                r = 1;
                break;
            }

            if (!pv->installed()) {
                WPMUtils::outputTextConsole("Package is not installed\n", false);
                r = 0;
                break;
            }

            if (pv->isExternal()) {
                WPMUtils::outputTextConsole("Externally installed packages cannot be removed\n",
                        false);
                r = 1;
                break;
            }

            QList<InstallOperation*> ops;
            QList<PackageVersion*> installed =
                    Repository::getDefault()->getInstalled();
            QString err = pv->planUninstallation(installed, ops);
            if (!err.isEmpty()) {
                WPMUtils::outputTextConsole(err + "\n", false);
                r = 1;
                break;
            }

            Job* job = createJob();
            rep->process(job, ops);
            if (!job->getErrorMessage().isEmpty()) {
                WPMUtils::outputTextConsole(job->getErrorMessage() + "\n", false);
                r = 1;
            } else {
                WPMUtils::outputTextConsole("The package was removed successfully\n");
            }
            delete job;
        } while (false);
    }

    return r;
}

int App::info()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = createJob();
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        WPMUtils::outputTextConsole(job->getErrorMessage() + "\n", false);
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString version = cl.get("version");

    if (r == 0) {
        if (package.isNull()) {
            WPMUtils::outputTextConsole("Missing option: --package\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        if (version.isNull()) {
            WPMUtils::outputTextConsole("Missing option: --version\n", false);
            r = 1;
        }
    }

    if (r == 0) {
        do {
            if (!Package::isValidName(package)) {
                WPMUtils::outputTextConsole("Invalid package name: " +
                        package + "\n", false);
                r = 1;
                break;
            }

            Repository* rep = Repository::getDefault();
            QList<Package*> packages = rep->findPackages(package);
            if (packages.count() == 0) {
                WPMUtils::outputTextConsole("Unknown package: " +
                        package + "\n", false);
                r = 1;
                break;
            }
            if (packages.count() > 1) {
                WPMUtils::outputTextConsole("Ambiguous package name\n", false);
                r = 1;
                break;
            }

            // debug: WPMUtils::outputTextConsole <<  package) << " " << versions);
            Version v;
            if (!v.setVersion(version)) {
                WPMUtils::outputTextConsole("Cannot parse version: " +
                        version + "\n", false);
                r = 1;
                break;
            }

            // debug: WPMUtils::outputTextConsole << "Versions: " << d.toString()) << std::endl;
            PackageVersion* pv = rep->findPackageVersion(
                    packages.at(0)->name, version);
            if (!pv) {
                WPMUtils::outputTextConsole("Package not found\n", false);
                r = 1;
                break;
            }

            Package* p = packages.at(0);
            WPMUtils::outputTextConsole("Icon: " +
                    p->icon + "\n");
            WPMUtils::outputTextConsole("Title: " +
                    pv->getPackageTitle() + "\n");
            WPMUtils::outputTextConsole("Version: " +
                    pv->version.getVersionString() + "\n");
            WPMUtils::outputTextConsole("Description: " +
                    p->description + "\n");
            WPMUtils::outputTextConsole("License: " +
                    p->license + "\n");
            WPMUtils::outputTextConsole("Installation path: " +
                    pv->getPath() + "\n");
            WPMUtils::outputTextConsole("Internal package name: " +
                    pv->package + "\n");
            WPMUtils::outputTextConsole("Status: " +
                    pv->getStatus() + "\n");
            WPMUtils::outputTextConsole("Download URL: " +
                    pv->download.toString() + "\n");
            WPMUtils::outputTextConsole("Package home page: " +
                    p->url + "\n");
            WPMUtils::outputTextConsole(QString("Type: ") +
                    (pv->type == 0 ? "zip" : "one-file") + "\n");
            WPMUtils::outputTextConsole("SHA1: " +
                    pv->sha1 + "\n");

            QString details;
            for (int i = 0; i < pv->importantFiles.count(); i++) {
                if (i != 0)
                    details.append("; ");
                details.append(pv->importantFilesTitles.at(i));
                details.append(" (");
                details.append(pv->importantFiles.at(i));
                details.append(")");
            }
            WPMUtils::outputTextConsole("Important files: " +
                    details + "\n");

            details = "";
            for (int i = 0; i < pv->dependencies.count(); i++) {
                if (i != 0)
                    details.append("; ");
                Dependency* d = pv->dependencies.at(i);
                details.append(d->toString());
            }
            WPMUtils::outputTextConsole("Dependencies: " +
                    details + "\n");

        } while (false);
    }

    return r;
}
