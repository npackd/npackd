#include <limits>
#include <math.h>
#include <QRegExp>

#include "ndapp.h"
#include "updatesearcher.h"
#include "..\wpmcpp\wpmutils.h"
#include "..\wpmcpp\commandline.h"
#include "..\wpmcpp\downloader.h"

int App::process()
{
    QString err = cl.parse();
    if (!err.isEmpty()) {
        WPMUtils::outputTextConsole("Error: " + err + "\n");
        return 1;
    }
    // cl.dump();

    int r = 0;

    QStringList fr = cl.getFreeArguments();

    if (fr.count() == 0) {
        WPMUtils::outputTextConsole("Missing command. Try nd help\n", false);
        r = 1;
    } else if (fr.count() > 1) {
        WPMUtils::outputTextConsole("Unexpected argument: " + fr.at(1) + "\n", false);
        r = 1;
    } else {
        const QString cmd = fr.at(0);
        if (cmd == "help") {
            usage();
        } else if (cmd == "find-updates") {
            r = findUpdates();
        } else {
            WPMUtils::outputTextConsole("Wrong command: " + cmd + "\n", false);
            r = 1;
        }
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
        r->updateNpackdCLEnvVar();
    }
}

void App::usage()
{
    const char* lines[] = {
        "Npackd command line tool",
        "Usage:",
        "    npackdcl help",
        "        prints this help",
        "    npackdcl add --package=<package> [--version=<version>]",
        "        installs a package. Short package names can be used here",
        "        (e.g. App instead of com.example.App)",
        "    npackdcl remove --package=<package> --version=<version>",
        "        removes a package. Short package names can be used here",
        "        (e.g. App instead of com.example.App)",
        "    npackdcl update --package=<package>",
        "        updates a package by uninstalling the currently installed",
        "        and installing the newest version. ",
        "        Short package names can be used here",
        "        (e.g. App instead of com.example.App)",
        "    npackdcl list [--status=installed | all] [--bare-format]",
        "        lists package versions sorted by package name and version.",
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
        "The process exits with the code unequal to 0 if an error occures.",
        "If the output is redirected, the texts will be encoded as UTF-8.",
    };
    for (int i = 0; i < (int) (sizeof(lines2) / sizeof(lines2[0])); i++) {
        WPMUtils::outputTextConsole(QString(lines2[i]) + "\n");
    }
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

int App::findUpdates()
{
    int r = 0;

    Job* job = clp.createJob();
    UpdateSearcher us;
    us.findUpdates(job);
    if (!job->getErrorMessage().isEmpty()) {
        WPMUtils::outputTextConsole(job->getErrorMessage() + "\n", false);
        r = 1;
    }
    delete job;

    return r;
}

