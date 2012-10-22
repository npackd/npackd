#include "updatesearcher.h"

#include <QTemporaryFile>

#include "..\wpmcpp\downloader.h"
#include "..\wpmcpp\wpmutils.h"
#include "..\wpmcpp\repository.h"
#include "..\wpmcpp\version.h"

class DiscoveryInfo {
public:
    QString package;
    QString versionPage;
    QString versionRE;
    QString downloadTemplate;
    UpdateSearcher::DownloadType dt;
    bool searchForMaxVersion;

    DiscoveryInfo(const QString& package,
            const QString& versionPage, const QString& versionRE,
            const QString& downloadTemplate,
            UpdateSearcher::DownloadType dt=UpdateSearcher::DT_STABLE,
            bool searchForMaxVersion=false);
};

DiscoveryInfo::DiscoveryInfo(const QString &package, const QString &versionPage,
        const QString &versionRE, const QString &downloadTemplate,
        UpdateSearcher::DownloadType dt, bool searchForMaxVersion)
{
    this->package = package;
    this->versionPage = versionPage;
    this->versionRE = versionRE;
    this->downloadTemplate = downloadTemplate;
    this->dt = dt;
    this->searchForMaxVersion = searchForMaxVersion;
}

UpdateSearcher::UpdateSearcher()
{
}

void UpdateSearcher::setDownload(Job* job, PackageVersion* pv,
        const QString& download)
{
    job->setHint("Downloading the package binary");

    Job* sub = job->newSubJob(0.9);
    QString sha1;

    QTemporaryFile* tf = Downloader::download(sub, QUrl(download), &sha1);
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
            setDownload(sub, ret, ret->download.toEncoded());
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
            setDownload(sub, ret, ret->download.toEncoded());
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
            setDownload(sub, ret, ret->download.toEncoded());
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
            setDownload(sub, ret, ret->download.toEncoded());
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
            setDownload(sub, ret, ret->download.toEncoded());
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
            setDownload(sub, ret, ret->download.toEncoded());
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
            setDownload(sub, ret, ret->download.toEncoded());
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

PackageVersion* UpdateSearcher::findUpdatesSimple(Job* job,
        const QString& package,
        const QString& versionPage, const QString& versionRE,
        const QString& downloadTemplate,
        Repository* templ,
        DownloadType dt, bool searchForMaxVersion)
{
    job->setHint("Preparing");

    PackageVersion* ret = 0;

    QString version;
    if (job->shouldProceed("Searching for updates")) {
        Job* sub = job->newSubJob(0.2);
        ret = findUpdate(sub, package,
                versionPage,
                versionRE, &version, searchForMaxVersion);
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
                    Version());
            if (t) {
                t = t->clone();
                t->version = ret->version;
                delete ret;
                ret = t;

                QString v = version;
                v.remove('.');
                QString version2Parts = ret->version.getVersionString(2);
                QString version3Parts = ret->version.getVersionString(3);
                QString version2PartsWithoutDots = version2Parts;
                version2PartsWithoutDots.remove('.');
                QString actualVersionWithUnderscores = version;
                actualVersionWithUnderscores.replace('.', '_');

                QMap<QString, QString> vars;
                vars.insert("version", ret->version.getVersionString());
                vars.insert("version2Parts", version2Parts);
                vars.insert("version3Parts", version3Parts);
                vars.insert("version2PartsWithoutDots", version2PartsWithoutDots);
                vars.insert("actualVersion", version);
                vars.insert("actualVersionWithoutDots", v);
                vars.insert("actualVersionWithUnderscores",
                        actualVersionWithUnderscores);
                ret->download  = WPMUtils::format(downloadTemplate, vars);
                job->setProgress(0.35);
            } else {
                job->setErrorMessage("package version not found in Template.xml");
            }
        } else {
            job->setProgress(0.35);
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (ret) {
            job->setHint("Examining the binary");

            if (!ret->sha1.isEmpty()) {
                Job* sub = job->newSubJob(0.65);
                setDownload(sub, ret, ret->download.toEncoded());
                if (!sub->getErrorMessage().isEmpty())
                    job->setErrorMessage(QString("Error downloading the package binary: %1").
                            arg(sub->getErrorMessage()));
                delete sub;

                QString fn = ret->download.path();
                QStringList parts = fn.split('/');
                fn = parts.at(parts.count() - 1);
                int last = fn.lastIndexOf(".");
                QString ext = fn.mid(last);

                if (dt == DT_GOOGLECODE)
                    ret->download = "http://npackd.googlecode.com/files/" +
                            ret->package + "-" +
                            ret->version.getVersionString() + ext;
                else if (dt == DT_DROPBOX)
                    ret->download = "http://dl.dropbox.com/u/17046326/files/" +
                            ret->package + "-" +
                            ret->version.getVersionString() + ext;
                else if (dt == DT_SOURCEFORGE)
                    ret->download = "http://downloads.sourceforge.net/project/npackd/" +
                            ret->package + "-" +
                            ret->version.getVersionString() + ext;
            } else {
                job->setProgress(1);
            }
        }
    }

    job->complete();

    return ret;
}

void UpdateSearcher::findUpdates(Job* job)
{
    Repository* rep = Repository::getDefault();
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Downloading repositories");

        Job* sub = job->newSubJob(0.29);
        rep->reload(sub, false);
        if (!sub->getErrorMessage().isEmpty()) {
            job->setErrorMessage(sub->getErrorMessage());
        }
        delete sub;
    }

    if (job->shouldProceed("Loading FoundPackages.xml")) {
        QFileInfo fi("FoundPackages.xml");
        if (fi.exists()) {
            Job* sub = job->newSubJob(0.1);
            rep->loadOne("FoundPackages.xml", sub);
            if (!sub->getErrorMessage().isEmpty()) {
                job->setErrorMessage(sub->getErrorMessage());
            }
            delete sub;
        } else {
            job->setProgress(0.39);
        }
    }

    QList<DiscoveryInfo> dis;
    dis.append(DiscoveryInfo("org.graphicsmagick.GraphicsMagickQ16",
            "http://sourceforge.net/api/file/index/project-id/73485/mtime/desc/limit/20/rss",
            "GraphicsMagick\\-([\\d\\.]+)\\.tar\\.gz",
            "http://downloads.sourceforge.net/project/graphicsmagick/graphicsmagick-binaries/"
            "${{version}}"
            "/GraphicsMagick-"
            "${{version}}"
            "-Q16-windows-dll.exe"));
    dis.append(DiscoveryInfo("GTKPlusBundle",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("H2",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("fr.handbrake.HandBrake",
            "http://handbrake.fr/downloads.php",
            "The current release version is <b>([\\d\\.]+)</b>",
            "http://handbrake.fr/rotation.php?file=HandBrake-${{version}}-i686-Win_GUI.exe"));
    dis.append(DiscoveryInfo("ImgBurn",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("IrfanView",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("Firefox",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("AC3Filter",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("AdobeReader",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("SharpDevelop",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("XULRunner",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("Clementine",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("AdvancedInstallerFreeware",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("org.xapian.XapianCore",
            "http://xapian.org/",
            "latest stable version is ([\\d\\.]+)<",
            "http://oligarchy.co.uk/xapian/${{actualVersion}}/xapian-core-${{actualVersion}}.tar.gz"));
    dis.append(DiscoveryInfo("net.sourceforge.aria2.Aria2",
            "http://sourceforge.net/api/file/index/project-id/159897/mtime/desc/limit/20/rss",
            "stable/aria2-([\\d\\.]+)/",
            "http://downloads.sourceforge.net/project/aria2/stable/aria2-${{actualVersion}}/aria2-${{actualVersion}}-win-32bit-build1.zip"));
    dis.append(DiscoveryInfo("net.blat.Blat",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/32%20bit%20versions/Win2000%20and%20newer/blat${{actualVersionWithoutDots}}_32.full.zip"));
    dis.append(DiscoveryInfo("net.sourceforge.aria2.Aria2_64",
            "http://sourceforge.net/api/file/index/project-id/159897/mtime/desc/limit/20/rss",
            "stable/aria2-([\\d\\.]+)/",
            "http://downloads.sourceforge.net/project/aria2/stable/aria2-${{actualVersion}}/aria2-${{actualVersion}}-win-64bit-build1.zip"));
    dis.append(DiscoveryInfo("net.blat.Blat64",
            "http://www.blat.net/",
            "Blat v([\\d\\.]+)",
            "http://downloads.sourceforge.net/project/blat/Blat%20Full%20Version/64%20bit%20versions/blat${{actualVersionWithoutDots}}_64.full.zip"));
    dis.append(DiscoveryInfo("org.blender.Blender",
            "http://www.blender.org/download/get-blender/",
            "Blender ([\\d\\.]+[a-z]?) is the latest release",
            "http://download.blender.org/release/Blender${{version2Parts}}/blender-${{actualVersion}}-release-windows32.zip"));
    dis.append(DiscoveryInfo("org.blender.Blender64",
            "http://www.blender.org/download/get-blender/",
            "Blender ([\\d\\.]+[a-z]?) is the latest release",
            "http://download.blender.org/release/Blender${{version2Parts}}/blender-${{actualVersion}}-release-windows64.zip"));
    dis.append(DiscoveryInfo("com.piriform.CCleaner",
            "http://www.piriform.com/ccleaner/download",
            "<b>([\\d\\.]+)</b>",
            "http://download.piriform.com/ccsetup${{version2PartsWithoutDots}}.exe"));
    dis.append(DiscoveryInfo("se.cdburnerxp.CDBurnerXP",
            "http://cdburnerxp.se/en/download",
            "<small>\\(([\\d\\.]+)\\)</small>",
            "http://download.cdburnerxp.se/portable/CDBurnerXP-${{version}}.zip"));
    dis.append(DiscoveryInfo("se.cdburnerxp.CDBurnerXP64",
            "http://cdburnerxp.se/en/download",
            "<small>\\(([\\d\\.]+)\\)</small>",
            "http://download.cdburnerxp.se/portable/CDBurnerXP-x64-${{version}}.zip"));
    dis.append(DiscoveryInfo("com.googlecode.gitextensions.GitExtensions",
            "http://code.google.com/p/gitextensions/downloads/list",
            "Git Extensions ([\\d\\.]+) Windows installer",
            "http://gitextensions.googlecode.com/files/GitExtensions${{actualVersionWithoutDots}}Setup.msi"));
    dis.append(DiscoveryInfo(
            "com.donationcoder.FARR",
            "http://www.donationcoder.com/Software/Mouser/findrun/index.html",
            "Download v([\\d\\.]+)",
            "http://www.donationcoder.com/Software/Mouser/findrun/downloads/FindAndRunRobotSetup.exe",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "com.google.Chrome",
            "http://omahaproxy.appspot.com/all?csv=1",
            "win,stable,([\\d\\.]+),",
            "http://dl.google.com/tag/s/appguid%3D%7B8A69D345-D564-463C-AFF1-A69D9E530F96%7D%26iid%3D%7B0A558A36-0536-8B3F-0AE0-420553CC97F3%7D%26lang%3Den%26browser%3D4%26usagestats%3D0%26appname%3DGoogle%2520Chrome%26needsadmin%3DFalse/edgedl/chrome/install/GoogleChromeStandaloneEnterprise.msi"));
    dis.append(DiscoveryInfo(
            "org.cmake.CMake",
            "http://cmake.org/cmake/resources/software.html",
            "Latest Release \\(([\\d\\.]+)\\)",
            "http://www.cmake.org/files/v${{version2Parts}}/cmake-${{version}}-win32-x86.zip"));
    dis.append(DiscoveryInfo(
            "net.sourceforge.dcplusplus.DCPlusPlus",
            "http://sourceforge.net/api/file/index/project-id/40287/mtime/desc/limit/20/rss",
            "DCPlusPlus-([\\d\\.]+)\\.exe",
            "http://sourceforge.net/projects/dcplusplus/files/DC%2B%2B%20${{version}}/DCPlusPlus-${{version}}.zip",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "com.maniactools.FreeM4aToMP3Converter",
            "http://www.maniactools.com/soft/m4a-to-mp3-converter/index.shtml",
            "Free M4a to MP3 Converter ([\\d\\.]+) <",
            "http://www.maniactools.com/soft/m4a-to-mp3-converter/m4a-to-mp3-converter.exe",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "com.dvdvideosoft.FreeYouTubeToMP3ConverterInstaller",
            "http://www.dvdvideosoft.com/products/dvd/Free-YouTube-to-MP3-Converter.htm",
            "versionNumberProg\\\">([\\d\\.]+)<",
            "http://download.dvdvideosoft.com/FreeYouTubeToMP3Converter.exe",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "uk.co.babelstone.BabelMap",
            "http://www.babelstone.co.uk/software/babelmap.html",
            "BabelMap Version ([\\d\\.]+) ",
            "http://www.babelstone.co.uk/Software/BabelMap.zip",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "uk.co.babelstone.BabelMap",
            "http://www.babelstone.co.uk/software/babelmap.html",
            "BabelMap Version ([\\d\\.]+) ",
            "http://www.babelstone.co.uk/Software/BabelMap.zip",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "org.golang.Go",
            "http://code.google.com/p/go/downloads/list",
            "go([\\d\\.]+)\\.windows\\-386\\.msi",
            "http://go.googlecode.com/files/go${{version}}.windows-386.msi"));
    dis.append(DiscoveryInfo(
            "org.golang.Go64",
            "http://code.google.com/p/go/downloads/list",
            "go([\\d\\.]+)\\.windows\\-amd64\\.msi",
            "http://go.googlecode.com/files/go${{version}}.windows-amd64.msi"));
    dis.append(DiscoveryInfo(
            "net.sourceforge.jabref.JabRef",
            "http://sourceforge.net/api/file/index/project-id/92314/mtime/desc/limit/20/rss",
            "JabRef\\-([\\d\\.]+)\\-setup\\.exe",
            "http://sourceforge.net/projects/jabref/files/jabref/${{version}}/JabRef-${{version}}-setup.exe"));
    dis.append(DiscoveryInfo(
            "info.keepass.KeePassClassic",
            "http://sourceforge.net/api/file/index/project-id/95013/mtime/desc/limit/120/rss",
            "KeePass-(1\\.[\\d\\.]+)\\-Setup\\.exe",
            "http://sourceforge.net/projects/keepass/files/KeePass%201.x/${{version}}/KeePass-${{version}}-Setup.exe"));
    dis.append(DiscoveryInfo(
            "info.keepass.KeePass",
            "http://sourceforge.net/api/file/index/project-id/95013/mtime/desc/limit/120/rss",
            "KeePass-(2\\.[\\d\\.]+)\\-Setup\\.exe",
            "http://sourceforge.net/projects/keepass/files/KeePass%202.x/${{version}}/KeePass-${{version}}-Setup.exe"));
    dis.append(DiscoveryInfo(
            "org.freepascal.lazarus.Lazarus",
            "http://sourceforge.net/api/file/index/project-id/89339/mtime/desc/limit/120/rss",
            "lazarus-([\\d\\.]+)-fpc-([\\d\\.]+)-win32.exe",
            "http://sourceforge.net/projects/lazarus/files/Lazarus%20Windows%2032%20bits/Lazarus%20${{version}}/lazarus-${{version}}-fpc-2.6.0-win32.exe",
            DT_GOOGLECODE));
    dis.append(DiscoveryInfo(
            "org.libreoffice.LibreOffice",
            "http://www.libreoffice.org/download/?type=win-x86&lang=en-US",
            ">([\\d\\.]+)<",
            "http://ftp.uni-muenster.de/pub/software/DocumentFoundation/libreoffice/stable/${{actualVersion}}/win/x86/LibO_${{actualVersion}}_Win_x86_install_multi.msi",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo(
            "com.selenic.mercurial.Mercurial",
            "http://mercurial.selenic.com/release/windows/",
            "Mercurial\\-([\\d\\.]+)\\.exe",
            "http://mercurial.selenic.com/release/windows/Mercurial-${{version}}.exe",
            DT_STABLE, true));
    dis.append(DiscoveryInfo(
            "com.selenic.mercurial.Mercurial64",
            "http://mercurial.selenic.com/release/windows/",
            "Mercurial\\-([\\d\\.]+)\\-x64\\.exe",
            "http://mercurial.selenic.com/release/windows/Mercurial-${{version}}-x64.exe",
            DT_STABLE, true));
    dis.append(DiscoveryInfo(
            "com.getmiro.Miro",
            "http://www.getmiro.com/",
            "Version  - ([\\d\\.]+)",
            "http://ftp.osuosl.org/pub/pculture.org/Miro/win/Miro-${{actualVersion}}.exe",
            DT_STABLE));
    dis.append(DiscoveryInfo(
            "de.mp3tag.MP3Tag",
            "http://www.mp3tag.de/",
            "Mp3tag v([\\d\\.]+) - ",
            "http://download.mp3tag.de/mp3tagv${{actualVersionWithoutDots}}setup.exe",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "org.nodejs.NodeJS",
            "http://nodejs.org/download/",
            ">v([\\d\\.]+)<",
            "http://nodejs.org/dist/v${{version}}/node-v${{version}}-x86.msi"));
    dis.append(DiscoveryInfo(
            "org.nodejs.NodeJS64",
            "http://nodejs.org/download/",
            ">v([\\d\\.]+)<",
            "http://nodejs.org/dist/v${{version}}/x64/node-v${{version}}-x64.msi"));
    dis.append(DiscoveryInfo(
            "com.opera.Opera",
            "http://www.opera.com/browser/",
            "version ([\\d\\.]+) for Windows",
            "http://get-tsw-1.opera.com/pub/opera/win/${{actualVersionWithoutDots}}/int/Opera_${{actualVersionWithoutDots}}_int_Setup.exe"));
    dis.append(DiscoveryInfo(
            "net.sourceforge.pidgin.Pidgin",
            "http://www.pidgin.im/",
            ">([\\d\\.]+)<",
            "http://downloads.sourceforge.net/project/pidgin/Pidgin/${{version}}/pidgin-${{version}}.exe"));
    dis.append(DiscoveryInfo(
            "com.softwareok.QDir",
            "http://www.softwareok.com/?seite=Freeware/Q-Dir",
            "Q-Dir ([\\d\\.]+)<",
            "http://www.softwareok.com/Download/Q-Dir_Portable_Unicode.zip",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "com.softwareok.QDir64",
            "http://www.softwareok.com/?seite=Freeware/Q-Dir",
            "Q-Dir ([\\d\\.]+)<",
            "http://www.softwareok.com/Download/Q-Dir_Portable_x64.zip",
            DT_DROPBOX));
    dis.append(DiscoveryInfo(
            "com.nokia.QtCreatorInstaller",
            "http://qt.nokia.com/downloads",
            "Qt Creator ([\\d\\.]+) for Windows",
            "http://get.qt.nokia.com/qtcreator/qt-creator-win-opensource-${{actualVersion}}.exe"));
    dis.append(DiscoveryInfo(
            "net.sourceforge.realterm.RealtermInstaller",
            "http://sourceforge.net/api/file/index/project-id/67297/mtime/desc/limit/20/rss",
            "Realterm_([\\d\\.]+)_setup.exe",
            "http://sourceforge.net/projects/realterm/files/Realterm/${{version}}/Realterm_${{version}}_setup.exe"));
    dis.append(DiscoveryInfo(
            "net.sourceforge.smplayer.SMPlayer",
            "http://sourceforge.net/api/file/index/project-id/185512/mtime/desc/limit/120/rss",
            "http://sourceforge\\.net/projects/smplayer/files/SMPlayer/[\\d\\.]+/smplayer-([\\d\\.]+)-win32\\.exe",
            "http://downloads.sourceforge.net/project/smplayer/SMPlayer/${{actualVersion}}/smplayer-${{actualVersion}}-win32.exe"));
    dis.append(DiscoveryInfo(
            "net.sourceforge.smplayer.SMPlayer64",
            "http://sourceforge.net/api/file/index/project-id/185512/mtime/desc/limit/120/rss",
            "http://sourceforge\\.net/projects/smplayer/files/SMPlayer/[\\d\\.]+/smplayer-([\\d\\.]+)-win32\\.exe",
            "http://downloads.sourceforge.net/project/smplayer/SMPlayer/${{actualVersion}}/smplayer-${{actualVersion}}-x64.exe"));
    dis.append(DiscoveryInfo(
            "org.mozilla.Thunderbird",
            "http://www.mozilla.org/en-US/thunderbird/all.html",
            ">([\\d\\.]+)<",
            "http://mirror.informatik.uni-mannheim.de/pub/mirrors/mozilla.org/thunderbird/releases/${{actualVersion}}/win32/en-US/Thunderbird%20Setup%20${{actualVersion}}.exe",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo(
            "com.googlecode.tortoisegit.TortoiseGit",
            "http://code.google.com/p/tortoisegit/downloads/list",
            "TortoiseGit-([\\d\\.]+)-32bit.msi",
            "http://tortoisegit.googlecode.com/files/TortoiseGit-${{actualVersion}}-32bit.msi"));
    dis.append(DiscoveryInfo(
            "com.googlecode.tortoisegit.TortoiseGit64",
            "http://code.google.com/p/tortoisegit/downloads/list",
            "TortoiseGit-([\\d\\.]+)-64bit.msi",
            "http://tortoisegit.googlecode.com/files/TortoiseGit-${{actualVersion}}-64bit.msi"));
    dis.append(DiscoveryInfo(
            "org.sourceforge.ultradefrag.UltraDefrag",
            "http://sourceforge.net/api/file/index/project-id/199532/mtime/desc/limit/20/rss",
            "stable-release/[\\d\\.]+/ultradefrag-portable-([\\d\\.]+)\\.bin\\.i386\\.zip",
            "http://downloads.sourceforge.net/project/ultradefrag/stable-release/${{actualVersion}}/ultradefrag-portable-${{actualVersion}}.bin.i386.zip"));
    dis.append(DiscoveryInfo(
            "org.sourceforge.ultradefrag.UltraDefrag64",
            "http://sourceforge.net/api/file/index/project-id/199532/mtime/desc/limit/20/rss",
            "stable-release/[\\d\\.]+/ultradefrag-portable-([\\d\\.]+)\\.bin\\.amd64\\.zip",
            "http://downloads.sourceforge.net/project/ultradefrag/stable-release/${{actualVersion}}/ultradefrag-portable-${{actualVersion}}.bin.amd64.zip"));
    dis.append(DiscoveryInfo(
            "net.amsn-project.AMSN",
            "https://sourceforge.net/api/file/index/project-id/54091/mtime/desc/limit/20/rss",
            "http://sourceforge\\.net/projects/amsn/files/amsn/[\\d\\.]+/aMSN-([\\d\\.]+)-tcl85-windows-installer\\.exe/download",
            "http://downloads.sourceforge.net/project/amsn/amsn/${{actualVersion}}/aMSN-${{actualVersion}}-tcl85-windows-installer.exe"));
    dis.append(DiscoveryInfo(
            "org.apache.Ant",
            "http://ant.apache.org/bindownload.cgi",
            "Currently, Apache Ant ([\\d\\.]+) is the best available version",
            "http://archive.apache.org/dist/ant/binaries/apache-ant-${{actualVersion}}-bin.zip"));
    dis.append(DiscoveryInfo(
            "net.apexdc.ApexDCPlusPlus",
            "https://sourceforge.net/api/file/index/project-id/157957/mtime/desc/limit/20/rss",
            "http://sourceforge\\.net/projects/apexdc/files/ApexDC%2B%2B/[\\d\\.]+/ApexDC%2B%2B_([\\d\\.]+)_Setup\\.exe/download",
            "http://downloads.sourceforge.net/project/apexdc/ApexDC%2B%2B/${{actualVersion}}/ApexDC%2B%2B_${{actualVersion}}_Setup.exe"));
    dis.append(DiscoveryInfo("com.piriform.Defraggler",
            "http://www.piriform.com/defraggler/download",
            "<b>([\\d\\.]+)</b>",
            "http://download.piriform.com/dfsetup${{version2PartsWithoutDots}}.exe"));
    dis.append(DiscoveryInfo("ie.heidi.eraser.EraserInstaller",
            "https://sourceforge.net/api/file/index/project-id/37015/mtime/desc/limit/20/rss",
            "http://sourceforge\\.net/projects/eraser/files/Eraser%206/[\\d\\.]+/Eraser%20([\\d\\.]+)\\.exe/download",
            "http://downloads.sourceforge.net/project/eraser/Eraser%206/${{version3Parts}}/Eraser%20${{actualVersion}}.exe"));
    dis.append(DiscoveryInfo("net.srware.Iron",
            "http://www.srware.net/software_srware_iron_download.php",
            "<strong>([\\d\\.]+)</strong>",
            "http://www.srware.net/downloads/srware_iron.exe",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("org.apache.jakarta.JMeter",
            "http://jmeter.apache.org/download_jmeter.cgi",
            "Apache JMeter ([\\d\\.]+) ",
            "http://apache.mirror.clusters.cc//jmeter/binaries/apache-jmeter-${{actualVersion}}.zip",
            DT_GOOGLECODE));
    dis.append(DiscoveryInfo("net.sourceforge.mpc-hc.MediaPlayerClassicHomeCinema",
            "http://sourceforge.net/api/file/index/project-id/170561/mtime/desc/limit/20/rss",
            "http://sourceforge\\.net/projects/mpc-hc/files/MPC%20HomeCinema%20-%20Win32/MPC-HC_v[\\d\\.]+_x86/MPC-HC\\.([\\d\\.]+)\\.x86\\.zip/download",
            "http://downloads.sourceforge.net/project/mpc-hc/MPC%20HomeCinema%20-%20Win32/MPC-HC_v${{actualVersion}}_x86/MPC-HC.${{actualVersion}}.x86.zip"));
    dis.append(DiscoveryInfo("net.sourceforge.mpc-hc.MediaPlayerClassicHomeCinema64",
            "http://sourceforge.net/api/file/index/project-id/170561/mtime/desc/limit/20/rss",
            "http://sourceforge\\.net/projects/mpc-hc/files/MPC%20HomeCinema%20-%20x64/MPC-HC_v[\\d\\.]+_x64/MPC-HC\\.([\\d\\.]+)\\.x64\\.zip/download",
            "http://downloads.sourceforge.net/project/mpc-hc/MPC%20HomeCinema%20-%20x64/MPC-HC_v${{actualVersion}}_x64/MPC-HC.${{actualVersion}}.x64.zip"));
    dis.append(DiscoveryInfo("org.miktex.MiKTeX",
            "http://sunsite.informatik.rwth-aachen.de/ftp/pub/mirror/ctan/systems/win32/miktex/setup/",
            "basic-miktex-([\\d\\.]+)\\.exe",
            "http://sunsite.informatik.rwth-aachen.de/ftp/pub/mirror/ctan/systems/win32/miktex/setup/basic-miktex-${{actualVersion}}.exe",
            DT_SOURCEFORGE, true));
    dis.append(DiscoveryInfo("org.miktex.MiKTeX64",
            "http://sunsite.informatik.rwth-aachen.de/ftp/pub/mirror/ctan/systems/win32/miktex/setup/",
            "basic-miktex-([\\d\\.]+)-x64\\.exe",
            "http://sunsite.informatik.rwth-aachen.de/ftp/pub/mirror/ctan/systems/win32/miktex/setup/basic-miktex-${{actualVersion}}-x64.exe",
            DT_SOURCEFORGE, true));
    dis.append(DiscoveryInfo("org.miranda-im.MirandaIM",
            "https://code.google.com/p/miranda/downloads/list",
            "miranda-im-v([\\d\\.]+)-unicode\\.exe",
            "https://miranda.googlecode.com/files/miranda-im-v${{actualVersion}}-unicode.exe"));
    dis.append(DiscoveryInfo("org.miranda-im.MirandaIM64",
            "https://code.google.com/p/miranda/downloads/list",
            "miranda-im-v([\\d\\.]+)-x64\\.7z",
            "https://miranda.googlecode.com/files/miranda-im-v${{actualVersion}}-x64.7z"));
    dis.append(DiscoveryInfo("com.mysql.MySQLCommunityServer",
            "http://www.mysql.com/downloads/mysql/",
            "<h1>MySQL Community Server ([\\d\\.]+[a-z]?)</h1>",
            "http://mysql.mirrors.ovh.net/ftp.mysql.com/Downloads/MySQL-${{version2Parts}}/mysql-${{actualVersion}}-win32.msi",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("com.mysql.MySQLCommunityServer64",
            "http://www.mysql.com/downloads/mysql/",
            "<h1>MySQL Community Server ([\\d\\.]+[a-z]?)</h1>",
            "http://mysql.mirrors.ovh.net/ftp.mysql.com/Downloads/MySQL-${{version2Parts}}/mysql-${{actualVersion}}-winx64.msi",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("org.python.Python",
            "http://www.python.org/download/releases/",
            ">Python ([\\d\\.]+)<",
            "http://www.python.org/ftp/python/${{actualVersion}}/python-${{actualVersion}}.msi"));
    dis.append(DiscoveryInfo("org.python.Python64",
            "http://www.python.org/download/releases/",
            ">Python ([\\d\\.]+)<",
            "http://www.python.org/ftp/python/${{actualVersion}}/python-${{actualVersion}}.amd64.msi"));
    dis.append(DiscoveryInfo("com.nokia.QtSource",
            "http://qt.nokia.com/downloads",
            ">Qt libraries ([\\d\\.]+) for Windows",
            "http://releases.qt-project.org/qt4/source/qt-everywhere-opensource-src-${{actualVersion}}.zip"));
    dis.append(DiscoveryInfo("com.nokia.QtMinGWInstaller",
            "http://qt.nokia.com/downloads",
            ">Qt libraries ([\\d\\.]+) for Windows",
            "http://releases.qt-project.org/qt4/source/qt-win-opensource-${{actualVersion}}-mingw.exe"));
    dis.append(DiscoveryInfo("net.sourceforge.shareaza.Shareaza",
            "http://sourceforge.net/api/file/index/project-id/110672/mtime/desc/limit/20/rss",
            "http://sourceforge\\.net/projects/shareaza/files/Shareaza/Shareaza%20[\\d\\.]+/Shareaza_([\\d\\.]+)_Win32\\.exe/download",
            "http://downloads.sourceforge.net/project/shareaza/Shareaza/Shareaza%20${{actualVersion}}/Shareaza_${{actualVersion}}_Win32.exe"));
    dis.append(DiscoveryInfo("net.sourceforge.shareaza.Shareaza64",
            "http://sourceforge.net/api/file/index/project-id/110672/mtime/desc/limit/20/rss",
            "http://sourceforge\\.net/projects/shareaza/files/Shareaza/Shareaza%20[\\d\\.]+/Shareaza_([\\d\\.]+)_x64\\.exe/download",
            "http://downloads.sourceforge.net/project/shareaza/Shareaza/Shareaza%20${{actualVersion}}/Shareaza_${{actualVersion}}_x64.exe"));
    dis.append(DiscoveryInfo("org.areca-backup.ArecaBackup",
            "http://sourceforge.net/api/file/index/project-id/171505/mtime/desc/limit/20/rss",
            "areca-([\\d\\.]+)-windows-jre32.zip",
            "http://downloads.sourceforge.net/project/areca/areca-stable/areca-${{actualVersion}}/areca-${{actualVersion}}-windows-jre32.zip"));
    dis.append(DiscoveryInfo("org.areca-backup.ArecaBackup64",
            "http://sourceforge.net/api/file/index/project-id/171505/mtime/desc/limit/20/rss",
            "areca-([\\d\\.]+)-windows-jre32.zip",
            "http://downloads.sourceforge.net/project/areca/areca-stable/areca-${{actualVersion}}/areca-${{actualVersion}}-windows-jre64.zip"));
    dis.append(DiscoveryInfo("org.boost.Boost",
            "http://sourceforge.net/api/file/index/project-id/7586/mtime/desc/limit/120/rss",
            "/boost-binaries/([\\d\\.]+)/",
            "http://downloads.sourceforge.net/project/boost/boost/${{actualVersion}}/boost_${{actualVersionWithUnderscores}}.zip"));
    dis.append(DiscoveryInfo("org.mozilla.Firefox",
            "http://www.mozilla.org/en-US/firefox/all.html",
            "<td class=\"curVersion\" >([\\d\\.]+)</td>",
            "http://download.mozilla.org/?product=firefox-${{actualVersion}}&os=win&lang=en-US",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("net.sourceforge.audacity.Audacity",
            "http://audacity.sourceforge.net/?lang=en",
            "<h3>Download Audacity ([\\d\\.]+)</h3>",
            "http://audacity.googlecode.com/files/audacity-win-${{actualVersion}}.exe"));
    dis.append(DiscoveryInfo("com.clamwin.ClamWin",
            "http://sourceforge.net/api/file/index/project-id/105508/mtime/desc/limit/20/rss",
            "\\[/clamwin/[\\d\\.]+/clamwin-([\\d\\.]+)-setup.exe\\]",
            "http://downloads.sourceforge.net/project/clamwin/clamwin/${{actualVersion}}/clamwin-${{actualVersion}}-setup.exe"));
    dis.append(DiscoveryInfo("se.haxx.curl.CURL",
            "http://curl.haxx.se/",
            ">The most recent stable version of curl is version ([\\d\\.]+)<",
            "http://curl.haxx.se/gknw.net/${{actualVersion}}/dist-w32/curl-${{actualVersion}}-rtmp-ssh2-ssl-sspi-zlib-idn-static-bin-w32.zip",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("se.haxx.curl.CURL64",
            "http://curl.haxx.se/",
            ">The most recent stable version of curl is version ([\\d\\.]+)<",
            "http://curl.haxx.se/gknw.net/${{actualVersion}}/dist-w64/curl-${{actualVersion}}-rtmp-ssh2-ssl-sspi-zlib-winidn-static-bin-w64.7z",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("com.ghostscript.GhostScriptInstaller",
            "http://www.ghostscript.com/download/",
            ">Ghostscript ([\\d\\.]+)<",
            "http://downloads.ghostscript.com/public/gs${{actualVersionWithoutDots}}w32.exe"));
    dis.append(DiscoveryInfo("com.ghostscript.GhostScript64Installer",
            "http://www.ghostscript.com/download/",
            ">Ghostscript ([\\d\\.]+)<",
            "http://downloads.ghostscript.com/public/gs${{actualVersionWithoutDots}}w64.exe"));
    dis.append(DiscoveryInfo("net.java.dev.glassfish.GlassFishWebProfile",
            "http://glassfish.java.net/",
            "Open Source Edition ([\\d\\.]+)",
            "http://download.java.net/glassfish/${{actualVersion}}/release/glassfish-${{actualVersion}}-web-ml.zip"));
    dis.append(DiscoveryInfo("org.gnucash.GNUCash",
            "http://sourceforge.net/api/file/index/project-id/192/mtime/desc/limit/20/rss",
            "\\[/gnucash \\(stable\\)/[\\d\\.]+/gnucash-([\\d\\.]+)-setup.exe\\]",
            "http://downloads.sourceforge.net/project/gnucash/gnucash%20%28stable%29/${{actualVersion}}/gnucash-${{actualVersion}}-setup.exe",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("org.izarc.IZArc",
            "http://www.izarc.org/download.html",
            "<b>([\\d\\.]+)</b>",
            "http://www.izarc.org/download/IZArcInstall.exe",
            DT_DROPBOX));
    dis.append(DiscoveryInfo("org.pdfforge.PDFCreator",
            "http://www.pdfforge.org/download",
            "Download PDFCreator ([\\d\\.]+)",
            "http://blue.download.pdfforge.org/pdfcreator/${{actualVersion}}/PDFCreator-${{actualVersionWithUnderscores}}_setup.exe"));
    dis.append(DiscoveryInfo("net.icsharpcode.SharpDevelop",
            "https://sourceforge.net/api/file/index/project-id/17610/mtime/desc/limit/120/rss",
            "http://sourceforge.net/projects/sharpdevelop/files/SharpDevelop.+/SharpDevelop_([\\d\\.]+)_Setup.msi",
            "http://sourceforge.net/projects/sharpdevelop/files/SharpDevelop%204.x/${{version2Parts}}/SharpDevelop_${{actualVersion}}_Setup.msi"));
    dis.append(DiscoveryInfo("org.apache.subversion.Subversion",
            "http://www.sliksvn.com/en/download",
            "> ([\\d\\.]+)<",
            "http://www.sliksvn.com/pub/Slik-Subversion-${{actualVersion}}-win32.msi"));
    dis.append(DiscoveryInfo("org.apache.subversion.Subversion64",
            "http://www.sliksvn.com/en/download",
            "> ([\\d\\.]+)<",
            "http://www.sliksvn.com/pub/Slik-Subversion-${{actualVersion}}-x64.msi"));
    dis.append(DiscoveryInfo("org.apache.tomcat.Tomcat",
            "http://tomcat.apache.org/download-70.cgi",
            "<strong>([\\d\\.]+)</strong>",
            "http://ftp.halifax.rwth-aachen.de/apache/tomcat/tomcat-7/v${{actualVersion}}/bin/apache-tomcat-${{actualVersion}}.exe",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("org.apache.tomcat.Tomcat64",
            "http://tomcat.apache.org/download-70.cgi",
            "<strong>([\\d\\.]+)</strong>",
            "http://ftp.halifax.rwth-aachen.de/apache/tomcat/tomcat-7/v${{actualVersion}}/bin/apache-tomcat-${{actualVersion}}.exe",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("net.sourceforge.winrun4j.WinRun4j",
            "http://sourceforge.net/api/file/index/project-id/195634/mtime/desc/limit/20/rss",
            "winrun4J-([\\d\\.]+)\\.zip",
            "http://downloads.sourceforge.net/project/winrun4j/winrun4j/${{actualVersion}}/winrun4J-${{actualVersion}}.zip"));
    dis.append(DiscoveryInfo("net.winscp.WinSCP",
            "http://winscp.net/eng/download.php",
            ">WinSCP ([\\d\\.]+)<",
            "http://downloads.sourceforge.net/project/winscp/WinSCP/${{actualVersion}}/winscp${{actualVersionWithoutDots}}.zip"));
    dis.append(DiscoveryInfo("org.wireshark.Wireshark",
            "http://www.wireshark.org/download.html",
            "The current stable release of Wireshark is ([\\d\\.]+)\\.",
            "http://wiresharkdownloads.riverbed.com/wireshark/win32/Wireshark-win32-${{actualVersion}}.exe",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("org.wireshark.Wireshark64",
            "http://www.wireshark.org/download.html",
            "The current stable release of Wireshark is ([\\d\\.]+)\\.",
            "http://wiresharkdownloads.riverbed.com/wireshark/win64/Wireshark-win64-${{actualVersion}}.exe",
            DT_SOURCEFORGE));
    dis.append(DiscoveryInfo("org.xapian.XapianCore",
            "http://xapian.org/",
            "latest stable version is ([\\d\\.]+)<",
            "http://oligarchy.co.uk/xapian/${{actualVersion}}/xapian-core-${{actualVersion}}.tar.gz"));

    /*
    ${{version}}
    ${{version2Parts}}
    ${{version3Parts}}
    ${{version2PartsWithoutDots}}
    ${{actualVersion}}
    ${{actualVersionWithoutDots}}
    ${{actualVersionWithUnderscores}}
    ${{match}}
    */
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

    QStringList failedPackages;

    for (int i = 0; i < dis.count(); i++) {
        const DiscoveryInfo di = dis.at(i);
        const QString package = di.package;
        if (job->isCancelled())
            break;

        job->setHint(QString("Searching for updates for %1").arg(package));

        Job* sub = job->newSubJob(0.6 / dis.count());
        PackageVersion* pv = 0;
        switch (i) {
            case 1:
                pv = findGTKPlusBundleUpdates(sub);
                break;
            case 2:
                // TODO: remove
                break;
            case 4:
                pv = findImgBurnUpdates(sub);
                break;
            case 5:
                pv = findIrfanViewUpdates(sub);
                break;
            case 6:
                // TODO: remove
                break;
            case 7:
                pv = findAC3FilterUpdates(sub);
                break;
            case 8:
                pv = findAdobeReaderUpdates(sub);
                break;
            case 9:
                // TODO: remove
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
                break;
            default:
                pv = findUpdatesSimple(sub, di.package, di.versionPage,
                        di.versionRE, di.downloadTemplate, templ, di.dt,
                        di.searchForMaxVersion);
                break;
        }

        if (!sub->getErrorMessage().isEmpty()) {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            GetConsoleScreenBufferInfo(hConsole, &csbi);
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED |
                    FOREGROUND_INTENSITY);
            WPMUtils::outputTextConsole(package + ": " +
                    sub->getErrorMessage() + "\n", false);
            SetConsoleTextAttribute(hConsole, csbi.wAttributes);
            failedPackages.append(package);
        } else {
            job->setProgress(0.4 + 0.55 * (i + 1) / dis.count());
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
                QString("\nNumber of found new packages: %1\n").arg(
                found->packageVersions.count()));
        WPMUtils::outputTextConsole(
                QString("Number of failures: %1\n").arg(
                failedPackages.count()));
        WPMUtils::outputTextConsole(QString("Failed packages: %1\n").
                arg(failedPackages.join("\n")));
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
