#include "updatesearcher.h"

#include <QTemporaryFile>

#include "..\wpmcpp\downloader.h"
#include "..\wpmcpp\wpmutils.h"
#include "..\wpmcpp\repository.h"
#include "..\wpmcpp\version.h"

UpdateSearcher::UpdateSearcher()
{
}

void UpdateSearcher::setDownload(Job* job, PackageVersion* pv)
{
    job->setHint("Downloading the package binary");

    Job* sub = job->newSubJob(0.9);
    QString sha1;

    QTemporaryFile* tf = Downloader::download(sub, QUrl(pv->download), &sha1);
    if (!sub->getErrorMessage().isEmpty())
        job->setErrorMessage(QString("Error downloading %1: %2").
                arg(pv->download.toString()).arg(sub->getErrorMessage()));
    else {
        pv->sha1 = sha1;
        job->setProgress(1);
    }
    delete tf;

    delete sub;

    job->complete();
}

QString UpdateSearcher::findTextInPage(Job* job, const QString& url,
        const QString& regex)
{
    QStringList ret = this->findTextsInPage(job, url, regex);
    if (ret.count() > 0)
        return ret.at(0);
    else
        return "";
}

QStringList UpdateSearcher::findTextsInPage(Job* job, const QString& url,
        const QString& regex)
{
    QStringList ret;

    job->setHint(QString("Downloading %1").arg(url));

    QTemporaryFile* tf = 0;
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        Job* sub = job->newSubJob(0.5);
        tf = Downloader::download(sub, QUrl(url));
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error downloading the page %1: %2").
                    arg(url).arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (!tf->open())
            job->setErrorMessage(
                    QString("Error opening the file downloaded from %1").
                    arg(url));
        else
            job->setProgress(0.51);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching in the text");

        QRegExp re(regex);
        QTextStream in(tf);

        while(!in.atEnd()) {
            QString line = in.readLine();
            //WPMUtils::outputTextConsole(line + "\n");
            int pos = re.indexIn(line);
            if (pos >= 0) {
                ret.append(re.cap(1));
            }
        }

        if (ret.isEmpty()) {
            job->setErrorMessage(QString("The text %1 was not found").
                    arg(regex));
        }
    }

    delete tf;

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findUpdate(Job* job, const QString& package,
        const QString& versionPage,
        const QString& versionRE, QString* realVersion,
        bool searchForMaxVersion) {
    QString version;
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching for the version number");

        Job* sub = job->newSubJob(0.9);
        if (!searchForMaxVersion) {
            version = findTextInPage(sub, versionPage, versionRE);
            if (realVersion)
                *realVersion = version;
        } else {
            QStringList versions = findTextsInPage(sub, versionPage, versionRE);
            Version max_(0, 0);
            max_.normalize();
            for (int i = 0; i < versions.count(); i++) {
                Version v;
                if (v.setVersion(versions.at(i))) {
                    if (v > max_) {
                        max_ = v;
                        version = versions.at(i);
                    }
                }
            }
            if (realVersion)
                *realVersion = version;
        }
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for version number: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    PackageVersion* ret = 0;
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching for the newest package");
        QChar last = version.at(version.length() - 1);
        if (last >= 'a' && last <= 'z')
            version = version.mid(0, version.length() - 1) + "." +
                    (last.toAscii() - 'a' + '1');

        Version v;
        if (v.setVersion(version)) {
            v.normalize();
            Repository* rep = Repository::getDefault();
            PackageVersion* ni = rep->findNewestInstallablePackageVersion(package);
            if (!ni || ni->version.compare(v) < 0) {
                ret = new PackageVersion(package);
                ret->version = v;
            }
            job->setProgress(1);
        } else {
            job->setErrorMessage(QString("Invalid package version found: %1").
                    arg(version));
        }
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findGraphicsMagickUpdates(Job* job) {
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    const QString installScript =
            "for /f \"delims=\" %%x in ('dir /b *.exe') do set setup=%%x\n"
            "\"%setup%\" /SP- /VERYSILENT /SUPPRESSMSGBOXES /NOCANCEL /NORESTART /DIR=\"%CD%\"\n"
            "if %errorlevel% neq 0 exit /b %errorlevel%\n"
            "\n"
            "if \"%npackd_cl%\" equ \"\" set npackd_cl=..\\com.googlecode.windows-package-manager.NpackdCL-1\n"
            "set onecmd=\"%npackd_cl%\\npackdcl.exe\" \"path\" \"--package=com.googlecode.windows-package-manager.CLU\" \"--versions=[1, 2)\"\n"
            "for /f \"usebackq delims=\" %%x in (`%%onecmd%%`) do set clu=%%x\n"
            "\"%clu%\\clu\" add-path --path \"%CD%\"\n"
            "verify\n";
    const QString uninstallScript =
            "unins000.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART\n"
            "if %errorlevel% neq 0 exit /b %errorlevel%\n"
            "\n"
            "if \"%npackd_cl%\" equ \"\" set npackd_cl=..\\com.googlecode.windows-package-manager.NpackdCL-1\n"
            "set onecmd=\"%npackd_cl%\\npackdcl.exe\" \"path\" \"--package=com.googlecode.windows-package-manager.CLU\" \"--versions=[1, 2)\"\n"
            "for /f \"usebackq delims=\" %%x in (`%%onecmd%%`) do set clu=%%x\n"
            "\"%clu%\\clu\" remove-path --path \"%CD%\"\n"
            "verify\n";

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching for updates");

        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "org.graphicsmagick.GraphicsMagickQ16",
                "http://sourceforge.net/api/file/index/project-id/73485/mtime/desc/limit/20/rss",
                "GraphicsMagick\\-([\\d\\.]+)\\.tar\\.gz");
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.75);
            ret->download = QUrl(QString(
                    "http://downloads.sourceforge.net/project/graphicsmagick/graphicsmagick-binaries/") +
                    ret->version.getVersionString() +
                    "/GraphicsMagick-" +
                    ret->version.getVersionString() +
                    "-Q16-windows-dll.exe");
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Adding dependencies and scripts");

            ret->type = 1;

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);
            pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                    uninstallScript);
            ret->files.append(pvf);
            Dependency* d = new Dependency();
            d->package = "com.googlecode.windows-package-manager.NpackdCL";
            d->minIncluded = true;
            d->min.setVersion(1, 15, 7);
            d->min.normalize();
            d->maxIncluded = false;
            d->max.setVersion(2, 0);
            d->max.normalize();
            ret->dependencies.append(d);

            d = new Dependency();
            d->package = "com.googlecode.windows-package-manager.NpackdCL";
            d->minIncluded = true;
            d->min.setVersion(1, 0);
            d->min.normalize();
            d->maxIncluded = true;
            d->max.setVersion(1, 0);
            d->max.normalize();
            ret->dependencies.append(d);

            d = new Dependency();
            d->package = "com.googlecode.windows-package-manager.CLU";
            d->minIncluded = true;
            d->min.setVersion(1, 0);
            d->min.normalize();
            d->maxIncluded = false;
            d->max.setVersion(2, 0);
            d->max.normalize();
            ret->dependencies.append(d);
        }

        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findGTKPlusBundleUpdates(Job* job) {
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching for updates");

        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "org.gtk.GTKPlusBundle",
                "http://www.gtk.org/download/win32.php",
                "http://ftp\\.gnome\\.org/pub/gnome/binaries/win32/gtk\\+/[\\d\\.]+/gtk\\+\\-bundle_([\\d\\.]+).+win32\\.zip");
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            Job* sub = job->newSubJob(0.2);
            QString url = findTextInPage(sub,
                    "http://www.gtk.org/download/win32.php",
                    "(http://ftp\\.gnome\\.org/pub/gnome/binaries/win32/gtk\\+/([\\d\\.]+)/gtk\\+\\-bundle_.+win32\\.zip)");
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error searching for version number: %1").
                        arg(sub->getErrorMessage()));
            delete sub;

            if (job->getErrorMessage().isEmpty()) {
                ret->download = QUrl(url);
                if (!ret->download.isValid())
                    job->setErrorMessage(QString(
                            "Download URL is not valid: %1").arg(url));
            }
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.55);
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (ret) {
        const QString installScript =
                "if \"%npackd_cl%\" equ \"\" set npackd_cl=..\\com.googlecode.windows-package-manager.NpackdCL-1\n"
                "set onecmd=\"%npackd_cl%\\npackdcl.exe\" \"path\" \"--package=com.googlecode.windows-package-manager.CLU\" \"--versions=[1, 2)\"\n"
                "for /f \"usebackq delims=\" %%x in (`%%onecmd%%`) do set clu=%%x\n"
                "\"%clu%\\clu\" add-path --path \"%CD%\\bin\"\n"
                "verify\n";
        const QString uninstallScript =
                "if \"%npackd_cl%\" equ \"\" set npackd_cl=..\\com.googlecode.windows-package-manager.NpackdCL-1\n"
                "set onecmd=\"%npackd_cl%\\npackdcl.exe\" \"path\" \"--package=com.googlecode.windows-package-manager.CLU\" \"--versions=[1, 2)\"\n"
                "for /f \"usebackq delims=\" %%x in (`%%onecmd%%`) do set clu=%%x\n"
                "\"%clu%\\clu\" remove-path --path \"%CD%\\bin\"\n"
                "verify\n";

        PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                installScript);
        ret->files.append(pvf);
        pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                uninstallScript);
        ret->files.append(pvf);
        pvf = new PackageVersionFile("etc\\gtk-2.0\\gtkrc",
                "gtk-theme-name = \"MS-Windows\"");
        ret->files.append(pvf);

        Dependency* d = new Dependency();
        d->package = "com.googlecode.windows-package-manager.NpackdCL";
        d->minIncluded = true;
        d->min.setVersion(1, 15, 7);
        d->min.normalize();
        d->maxIncluded = false;
        d->max.setVersion(2, 0);
        d->max.normalize();
        ret->dependencies.append(d);

        d = new Dependency();
        d->package = "com.googlecode.windows-package-manager.NpackdCL";
        d->minIncluded = true;
        d->min.setVersion(1, 0);
        d->min.normalize();
        d->maxIncluded = true;
        d->max.setVersion(1, 0);
        d->max.normalize();
        ret->dependencies.append(d);

        d = new Dependency();
        d->package = "com.googlecode.windows-package-manager.CLU";
        d->minIncluded = true;
        d->min.setVersion(1, 0);
        d->min.normalize();
        d->maxIncluded = false;
        d->max.setVersion(2, 0);
        d->max.normalize();
        ret->dependencies.append(d);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findH2Updates(Job* job) {
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching for updates");

        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "com.h2database.H2",
                "http://www.h2database.com/html/download.html",
                "Version ([\\d\\.]+) \\([\\d\\-]+\\), Last Stable");
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            Job* sub = job->newSubJob(0.2);
            QString url = findTextInPage(sub,
                    "http://www.h2database.com/html/download.html",
                    "Version [\\d\\.]+ \\(([\\d\\-]+)\\), Last Stable");
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error searching for version number: %1").
                        arg(sub->getErrorMessage()));
            delete sub;

            if (job->getErrorMessage().isEmpty()) {
                ret->download = QUrl("http://www.h2database.com/h2-setup-" +
                        url + ".exe");
                if (!ret->download.isValid())
                    job->setErrorMessage(QString(
                            "Download URL is not valid: %1").arg(url));
            }
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.55);
            ret->type = 1;
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (ret) {
        const QString installScript =
                "for /f \"delims=\" %%x in ('dir /b *.exe') do set setup=%%x\n"
                "\"%setup%\" /S /D=%CD%\\Program\n";
        const QString uninstallScript =
                "Program\\Uninstall.exe /S _?=%CD%\\Program\n";

        PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                installScript);
        ret->files.append(pvf);
        pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                uninstallScript);
        ret->files.append(pvf);

        Dependency* d = new Dependency();
        d->package = "com.oracle.JRE";
        d->minIncluded = true;
        d->min.setVersion(1, 5);
        d->min.normalize();
        d->maxIncluded = false;
        d->max.setVersion(2, 0);
        d->max.normalize();
        ret->dependencies.append(d);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findHandBrakeUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching for updates");

        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "fr.handbrake.HandBrake",
                "http://handbrake.fr/downloads.php",
                "The current release version is <b>([\\d\\.]+)</b>");
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            ret->download = QUrl("http://handbrake.fr/rotation.php?file=HandBrake-" +
                    ret->version.getVersionString() + "-i686-Win_GUI.exe");
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.55);
            ret->type = 1;
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (ret) {
        const QString installScript =
                "ren rotation.php setup.exe\n"
                "setup.exe /S /D=%CD%\n";
        const QString uninstallScript =
                "uninst.exe /S\n";

        PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                installScript);
        ret->files.append(pvf);
        pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                uninstallScript);
        ret->files.append(pvf);

        Dependency* d = new Dependency();
        d->package = "com.microsoft.DotNetRedistributable";
        d->minIncluded = true;
        d->min.setVersion(2, 0);
        d->min.normalize();
        d->maxIncluded = false;
        d->max.setVersion(3, 0);
        d->max.normalize();
        ret->dependencies.append(d);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findImgBurnUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching for updates");

        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "com.imgburn.ImgBurn",
                "http://www.imgburn.com/",
                "Current version: ([\\d\\.]+)", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            ret->download = QUrl("http://download.imgburn.com/SetupImgBurn_" +
                    version + ".exe");
        }
        job->setProgress(0.3);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            ret->type = 1;
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (job->shouldProceed("Setting scripts")) {
        if (ret) {
            const QString installScript =
                    "for /f %%x in ('dir /b *.exe') do set setup=%%x\n"
                    "\"%setup%\" /S /D=%CD%\n";
            const QString uninstallScript =
                    "uninstall.exe /S\n";

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);
            pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                    uninstallScript);
            ret->files.append(pvf);
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findIrfanViewUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Searching for updates");

        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "de.irfanview.IrfanView",
                "http://www.irfanview.net/main_start_engl.htm",
                "Current version: ([\\d\\.]+)", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            ret->download = QUrl("http://irfanview.tuwien.ac.at/iview" +
                    version.remove('.') + ".zip");
        }
        job->setProgress(0.3);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            ret->type = 1;
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (job->shouldProceed("Setting scripts")) {
        if (ret) {
            const QString installScript =
                    "for /f %%x in ('dir /b *.exe') do set setup=%%x\n"
                    "\"%setup%\" /S /D=%CD%\n";
            const QString uninstallScript =
                    "uninstall.exe /S\n";

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);
            pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                    uninstallScript);
            ret->files.append(pvf);
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findFirefoxUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "org.mozilla.Firefox",
                "http://www.mozilla.org/en-US/firefox/all.html",
                "<td class=\"curVersion\" >([\\d\\.]+)</td>", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            ret->type = 1;
            ret->download = QUrl("http://download.mozilla.org/?product=firefox-" +
                    version + "&os=win&lang=en-US");
        }
        job->setProgress(0.3);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            ret->type = 1;
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (job->shouldProceed("Setting scripts")) {
        if (ret) {
            ret->download = QUrl("http://npackd.googlecode.com/files/org.mozilla.Firefox-" +
                    ret->version.getVersionString() + ".exe");
            const QString installScript =
                    "echo [Install] > .WPM\\FF.ini\n"
                    "echo InstallDirectoryPath=%CD% >> .WPM\\FF.ini\n"
                    "echo QuickLaunchShortcut=false >> .WPM\\FF.ini\n"
                    "echo DesktopShortcut=false >> .WPM\\FF.ini\n"
                    "for /f \"delims=\" %%x in ('dir /b *.exe') do set setup=%%x\n"
                    "\"%setup%\" /INI=\"%CD%\\.WPM\\FF.ini\"\n";
            const QString uninstallScript =
                    "uninstall\\helper.exe /S\n";

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);
            pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                    uninstallScript);
            ret->files.append(pvf);

            ret->importantFiles.append("firefox.exe");
            ret->importantFilesTitles.append("Firefox");
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findAC3FilterUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "net.ac3filter.AC3Filter",
                "http://ac3filter.net/wiki/Download_AC3Filter",
                ">AC3Filter ([\\d\\.]+[a-z]?)<", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            ret->type = 1;
            version.replace('.', '_');
            ret->download = QUrl("http://ac3filter.googlecode.com/files/ac3filter_" +
                    version + ".exe");
        }
        job->setProgress(0.3);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            ret->type = 1;
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (job->shouldProceed("Setting scripts")) {
        if (ret) {
            const QString installScript =
                    "for /f \"delims=\" %%x in ('dir /b *.exe') do set setup=%%x\n"
                    "\"%setup%\" /SP- /VERYSILENT /SUPPRESSMSGBOXES /NOCANCEL /NORESTART /DIR=\"%CD%\" /SAVEINF=\"%CD%\\.WPM\\InnoSetupInfo.ini\" /LOG=\"%CD%\\.WPM\\InnoSetupInstall.log\"\n"
                    "set err=%errorlevel%\n"
                    "type .WPM\\InnoSetupInstall.log\n"
                    "if %err% neq 0 exit %err%\n";
            const QString uninstallScript =
                    "unins000.exe /VERYSILENT /SUPPRESSMSGBOXES /NORESTART /LOG=\"%CD%\\.WPM\\InnoSetupUninstall.log\"\n"
                    "set err=%errorlevel%\n"
                    "type .WPM\\InnoSetupUninstall.log\n"
                    "if %err% neq 0 exit %err%\n";

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);
            pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                    uninstallScript);
            ret->files.append(pvf);
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findAdobeReaderUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "com.adobe.AdobeReader",
                "http://www.filehippo.com/download_adobe_reader/",
                "<span itemprop=\"name\">Adobe Reader ([\\d\\.]+)</span>", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            ret->type = 1;
            QString sv = version;
            sv.remove('.');
            ret->download = QUrl("http://ardownload.adobe.com/pub/adobe/reader/win/10.x/" +
                    version + "/en_US/AdbeRdr" +
                    sv +
                    "_en_US.exe");
        }
        job->setProgress(0.3);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            ret->type = 1;
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (job->shouldProceed("Setting scripts")) {
        if (ret) {
            const QString installScript =
                    "for /f \"delims=\" %%x in ('dir /b *.exe') do set setup=%%x\n"
                    "\"%setup%\" /sAll /rs /msi /Lime \"%CD%\\.WPM\\MSI.log\" ALLUSERS=1 INSTALLDIR=\"%CD%\"\n"
                    "set err=%errorlevel%\n"
                    "type .WPM\\MSI.log\n"
                    "rem 3010=restart required\n"
                    "if %err% equ 3010 exit /b 0\n"
                    "if %err% neq 0 exit /b %err%\n"
                    "\n"
                    "rem disable automatic updates\n"
                    "if \"%ProgramFiles(x86)%\" NEQ \"\" goto x64\n"
                    "REG ADD \"HKLM\\SOFTWARE\\Adobe\\Adobe ARM\\1.0\\ARM\" /f /v iCheckReader /t REG_DWORD /d 0\n"
                    "if %errorlevel% neq 0 exit /b %errorlevel%\n"
                    "goto :eof\n"
                    "\n"
                    ":x64\n"
                    "REG ADD \"HKLM\\SOFTWARE\\Wow6432Node\\Adobe\\Adobe ARM\\1.0\\ARM\" /f /v iCheckReader /t REG_DWORD /d 0\n"
                    "if %errorlevel% neq 0 exit /b %errorlevel%\n"
                    "goto :eof\n";
            const QString uninstallScript =
                    "msiexec.exe /qn /norestart /Lime .WPM\\MSI.log /X{AC76BA86-7AD7-1033-7B44-AA1000000001}\n"
                    "set err=%errorlevel%\n"
                    "type .WPM\\MSI.log\n"
                    "if %err% neq 0 exit %err%\n";

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);
            pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                    uninstallScript);
            ret->files.append(pvf);
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findSharpDevelopUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "net.icsharpcode.SharpDevelop",
                "https://sourceforge.net/api/file/index/project-id/17610/mtime/desc/limit/120/rss",
                "http://sourceforge.net/projects/sharpdevelop/files/SharpDevelop.+/SharpDevelop_([\\d\\.]+)_Setup.msi", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            ret->type = 1;
            Job* sub = job->newSubJob(0.2);
            QString url = findTextInPage(sub,
                    "https://sourceforge.net/api/file/index/project-id/17610/mtime/desc/limit/120/rss",
                    "(http://sourceforge.net/projects/sharpdevelop/files/SharpDevelop.+/SharpDevelop_[\\d\\.]+_Setup.msi)");
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error searching for the newest version: %1").
                        arg(sub->getErrorMessage()));
            delete sub;

            if (job->getErrorMessage().isEmpty())
                ret->download = QUrl(url);
        }
        job->setProgress(0.3);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            ret->type = 1;
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (job->shouldProceed("Setting scripts")) {
        if (ret) {
            const QString installScript =
                    "if \"%npackd_cl%\" equ \"\" set npackd_cl=..\\com.googlecode.windows-package-manager.NpackdCL-1\n"
                    "set onecmd=\"%npackd_cl%\\npackdcl.exe\" \"path\" \"--package=com.googlecode.windows-package-manager.NpackdInstallerHelper\" \"--versions=[1, 1]\"\n"
                    "for /f \"usebackq delims=\" %%x in (`%%onecmd%%`) do set npackdih=%%x\n"
                    "call \"%npackdih%\\InstallMSI.bat\" INSTALLDIR\n";
            const QString uninstallScript =
                    "if \"%npackd_cl%\" equ \"\" set npackd_cl=..\\com.googlecode.windows-package-manager.NpackdCL-1\n"
                    "set onecmd=\"%npackd_cl%\\npackdcl.exe\" \"path\" \"--package=com.googlecode.windows-package-manager.NpackdInstallerHelper\" \"--versions=[1, 1]\"\n"
                    "for /f \"usebackq delims=\" %%x in (`%%onecmd%%`) do set npackdih=%%x\n"
                    "call \"%npackdih%\\UninstallMSI.bat\n";

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);
            pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                    uninstallScript);
            ret->files.append(pvf);

            Dependency* d = new Dependency();
            d->package = "com.googlecode.windows-package-manager.NpackdCL";
            d->minIncluded = true;
            d->min.setVersion(1, 15, 7);
            d->min.normalize();
            d->maxIncluded = false;
            d->max.setVersion(2, 0);
            d->max.normalize();
            ret->dependencies.append(d);

            d = new Dependency();
            d->package = "com.googlecode.windows-package-manager.NpackdCL";
            d->minIncluded = true;
            d->min.setVersion(1, 0);
            d->min.normalize();
            d->maxIncluded = true;
            d->max.setVersion(1, 0);
            d->max.normalize();
            ret->dependencies.append(d);

            d = new Dependency();
            d->package = "com.googlecode.windows-package-manager.NpackdInstallerHelper";
            d->minIncluded = true;
            d->min.setVersion(1, 0);
            d->min.normalize();
            d->maxIncluded = true;
            d->max.setVersion(1, 0);
            d->max.normalize();
            ret->dependencies.append(d);

            d = new Dependency();
            d->package = "com.microsoft.DotNetRedistributable";
            d->minIncluded = true;
            d->min.setVersion(4, 0);
            d->min.normalize();
            d->maxIncluded = false;
            d->max.setVersion(5, 0);
            d->max.normalize();
            ret->dependencies.append(d);
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findXULRunnerUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "org.mozilla.XULRunner",
                "http://ftp.mozilla.org/pub/mozilla.org/xulrunner/releases/",
                ">([\\d\\.]+)/", &version, true);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            QString url = "http://ftp.mozilla.org/pub/mozilla.org/xulrunner/releases/" +
                    version + "/runtimes/xulrunner-" +
                    version + ".en-US.win32.zip";
            ret->download = QUrl(url);
        }
        job->setProgress(0.3);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (job->shouldProceed("Setting scripts")) {
        if (ret) {
            const QString installScript =
                    "for /f \"delims=\" %%x in ('dir /b xulrunner*') do set name=%%x\n"
                    "cd \"%name%\"\n"
                    "for /f \"delims=\" %%a in ('dir /b') do (\n"
                    "  move \"%%a\" ..\n"
                    ")\n"
                    "cd ..\n"
                    "rmdir \"%name%\"\n";

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findClementineUpdates(Job* job)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, "org.clementine-player.Clementine",
                "http://code.google.com/p/clementine-player/downloads/list",
                "ClementineSetup-([\\d\\.]+)\\.exe", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            ret->type = 1;
            QString url = "http://clementine-player.googlecode.com/files/ClementineSetup-" +
                    version + ".exe";
            ret->download = QUrl(url);
        }
        job->setProgress(0.3);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    if (job->shouldProceed("Setting scripts")) {
        if (ret) {
            const QString installScript =
                    "for /f %%x in ('dir /b *.exe') do set setup=%%x\n"
                    "\"%setup%\" /S /D=%CD%\n"
                    "if %errorlevel% neq 0 exit /b %errorlevel%\n"
                    "\n"
                    "set package=org.clementine-player.Clementine\n"
                    "set version=" + version + "\n"
                    "set key=%package%-%version%\n"
                    "\n"
                    "reg add HKLM\\SOFTWARE\\Classes\\Applications\\%key%\\shell\\open /v FriendlyAppName /d \"Clementine (%version%)\" /f\n"
                    "set c=\\\"%CD%\\clementine.exe\\\" \\\"%%1\\\"\n"
                    "reg add HKLM\\SOFTWARE\\Classes\\Applications\\%KEY%\\shell\\open\\command /d \"%c%\" /f\n"
                    "for %%g in (mp3 ogg flac mpc m4a aac wma mp4 spx wav m3u m3u8 xspf pls asx asxini) do reg add HKLM\\SOFTWARE\\Classes\\.%%g\\OpenWithList\\%key% /f\n"
                    "verify\n";

            PackageVersionFile* pvf = new PackageVersionFile(".WPM\\Install.bat",
                    installScript);
            ret->files.append(pvf);

            pvf = new PackageVersionFile(".WPM\\Uninstall.bat",
                    "Uninstall.exe /S _?=%CD%\n"
                    "if %errorlevel% neq 0 exit /b %errorlevel%\n"
                    "\n"
                    "set package=org.clementine-player.Clementine\n"
                    "set version=" + version + "\n"
                    "set key=%package%-%version%\n"
                    "\n"
                    "reg delete HKLM\\SOFTWARE\\Classes\\Applications\\%key% /f\n"
                    "for %%g in (mp3 ogg flac mpc m4a aac wma mp4 spx wav m3u m3u8 xspf pls asx asxini) do reg delete HKLM\\SOFTWARE\\Classes\\.%%g\\OpenWithList\\%key% /f\n"
                    "verify\n");
            ret->files.append(pvf);

            ret->importantFiles.append("clementine.exe");
            ret->importantFilesTitles.append("Clementine");
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findAdvancedInstallerFreewareUpdates(Job* job,
        Repository* templ)
{
    job->setHint("Preparing");

    QString package = "com.advancedinstaller.AdvancedInstallerFreeware";
    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, package,
                "http://www.advancedinstaller.com/download.html",
                "\\\">([\\d\\.]+)</a>", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            PackageVersion* t = templ->findPackageVersion(
                    package,
                    Version())->clone();
            t->version = ret->version;
            delete ret;
            ret = t;
        }
        job->setProgress(1);
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findAria2Updates(Job* job,
        Repository* templ)
{
    job->setHint("Preparing");

    QString package = "net.sourceforge.aria2.Aria2";
    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, package,
                "http://aria2.sourceforge.net/",
                ">Get ([\\d\\.]+)</a>", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            PackageVersion* t = templ->findPackageVersion(
                    package,
                    Version())->clone();
            t->version = ret->version;
            delete ret;
            ret = t;

            ret->download  = QUrl("http://downloads.sourceforge.net/project/aria2/stable/aria2-" +
                    version + "/aria2-" + version + "-i686-w64-mingw32-build1.zip");
        }
        job->setProgress(0.35);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findBlatUpdates(Job* job,
        Repository* templ)
{
    job->setHint("Preparing");

    QString package = "net.blat.Blat";
    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, package,
                "http://www.blat.net/",
                "Blat v([\\d\\.]+)", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            PackageVersion* t = templ->findPackageVersion(
                    package,
                    Version())->clone();
            t->version = ret->version;
            delete ret;
            ret = t;

            QString v = version;
            v.remove('.');
            ret->download  = QUrl("http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat" +
                    v + "_32.full.zip");
        }
        job->setProgress(0.35);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    job->complete();

    return ret;
}

PackageVersion* UpdateSearcher::findAria2_64Updates(Job* job,
        Repository* templ)
{
    job->setHint("Preparing");

    QString package = "net.sourceforge.aria2.Aria2_64";
    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, package,
                "http://aria2.sourceforge.net/",
                ">Get ([\\d\\.]+)</a>", &version);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error searching for the newest version: %1").
                    arg(sub->getErrorMessage()));
        delete sub;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Searching for the download URL");

            PackageVersion* t = templ->findPackageVersion(
                    package,
                    Version())->clone();
            t->version = ret->version;
            delete ret;
            ret = t;

            ret->download  = QUrl(
                    "http://downloads.sourceforge.net/project/aria2/stable/aria2-" +
                    version + "/aria2-" + version + "-x86_64-w64-mingw32-build1.zip");
        }
        job->setProgress(0.35);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            Job* sub = job->newSubJob(0.65);
            setDownload(sub, ret);
            if (!sub->getErrorMessage().isEmpty())
                job->setErrorMessage(QString("Error downloading the package binary: %1").
                        arg(sub->getErrorMessage()));
            delete sub;
        }
    }

    job->complete();

    return ret;
}

void UpdateSearcher::findUpdates(Job* job)
{
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Downloading repositories");

        Repository* rep = Repository::getDefault();
        Job* sub = job->newSubJob(0.39);
        rep->reload(sub);
        if (!sub->getErrorMessage().isEmpty()) {
            job->setErrorMessage(sub->getErrorMessage());
        }
        delete sub;
    }

    QStringList packages;
    packages.append("GraphicsMagick");
    packages.append("GTKPlusBundle");
    packages.append("H2");
    packages.append("HandBrake");
    packages.append("ImgBurn");
    packages.append("IrfanView");
    packages.append("Firefox");
    packages.append("AC3Filter");
    packages.append("AdobeReader");
    packages.append("SharpDevelop");
    packages.append("XULRunner");
    packages.append("Clementine");
    packages.append("AdvancedInstallerFreeware");
    packages.append("Aria2");
    packages.append("Blat");
    packages.append("Aria2_64");

    Repository* templ = new Repository();
    if (job->shouldProceed("Reading the template repository")) {
        Job* sub = job->newSubJob(0.01);
        templ->loadOne("..\\nd\\Template.xml", sub);
        if (!sub->getErrorMessage().isEmpty()) {
            job->setErrorMessage(sub->getErrorMessage());
        }
        delete sub;
    }

    Repository* found = new Repository();

    for (int i = 0; i < packages.count(); i++) {
        const QString package = packages.at(i);
        if (job->isCancelled())
            break;

        job->setHint(QString("Searching for updates for %1").arg(package));

        Job* sub = job->newSubJob(0.6 / packages.count());
        PackageVersion* pv = 0;
        switch (i) {
            case 0:
                pv = findGraphicsMagickUpdates(sub);
                break;
            case 1:
                pv = findGTKPlusBundleUpdates(sub);
                break;
            case 2:
                pv = findH2Updates(sub);
                break;
            case 3:
                pv = findHandBrakeUpdates(sub);
                break;
            case 4:
                pv = findImgBurnUpdates(sub);
                break;
            case 5:
                pv = findIrfanViewUpdates(sub);
                break;
            case 6:
                pv = findFirefoxUpdates(sub);
                break;
            case 7:
                pv = findAC3FilterUpdates(sub);
                break;
            case 8:
                pv = findAdobeReaderUpdates(sub);
                break;
            case 9:
                pv = findSharpDevelopUpdates(sub);
                break;
            case 10:
                pv = findXULRunnerUpdates(sub);
                break;
            case 11:
                pv = findClementineUpdates(sub);
                break;
            case 12:
                pv = findAdvancedInstallerFreewareUpdates(sub, templ);
                break;
            case 13:
                pv = findAria2Updates(sub, templ);
                break;
            case 14:
                pv = findBlatUpdates(sub, templ);
                break;
            case 15:
                pv = findAria2_64Updates(sub, templ);
                break;
        }
        if (!sub->getErrorMessage().isEmpty()) {
            job->setErrorMessage(sub->getErrorMessage());
        } else {
            job->setProgress(0.4 + 0.55 * (i + 1) / packages.count());
            if (!pv)
                WPMUtils::outputTextConsole(QString(
                        "No updates found for %1\n").arg(package));
            else {
                WPMUtils::outputTextConsole(
                        QString("New %1 version: %2\n").arg(package).
                        arg(pv->version.getVersionString()));
                found->packageVersions.append(pv);
            }
        }
        delete sub;

        if (!job->getErrorMessage().isEmpty())
            break;
    }

    if (job->shouldProceed("Writing repository with found packages")) {
        WPMUtils::outputTextConsole(
                QString("Number of found new packages: %1\n").arg(
                found->packageVersions.count()));
        if (found->packageVersions.count() > 0) {
            QString err = found->writeTo("FoundPackages.xml");
            if (!err.isEmpty())
                job->setErrorMessage(err);
            else
                job->setProgress(1);
        } else {
            job->setProgress(1);
        }
    }

    delete found;
    delete templ;

    job->complete();
}
