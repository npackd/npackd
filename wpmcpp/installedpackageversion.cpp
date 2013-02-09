#include "installedpackageversion.h"

#include <QDir>

#include "windowsregistry.h"

InstalledPackageVersion::InstalledPackageVersion(const QString &package,
        const Version &version, const QString &directory)
{
    this->package = package;
    this->version = version;
    this->directory = directory;
}

QString InstalledPackageVersion::getDirectory() const
{
    return this->directory;
}

QString InstalledPackageVersion::getDetectionInfo() const
{
    return this->detectionInfo;
}

bool InstalledPackageVersion::installed() const
{
    return !this->getDirectory().isEmpty();
}

void InstalledPackageVersion::setPath(const QString& path)
{
    if (this->directory != path) {
        this->directory = path;
        this->save();
    }
}

QString InstalledPackageVersion::setDetectionInfo(const QString& info)
{
    QString r;
    if (this->detectionInfo != info) {
        this->detectionInfo = info;
        this->save();
    }
    return r;
}

QString InstalledPackageVersion::save() const
{
    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false);
    QString r;
    QString keyName = "SOFTWARE\\Npackd\\Npackd\\Packages";
    QString pn = this->package + "-" + this->version.getVersionString();
    if (!this->directory.isEmpty()) {
        WindowsRegistry wr = machineWR.createSubKey(keyName + "\\" + pn, &r);
        if (r.isEmpty()) {
            r = wr.set("Path", this->directory);
            if (r.isEmpty())
                r = wr.set("DetectionInfo", this->detectionInfo);

            // for compatibility with Npackd 1.16 and earlier. They
            // see all package versions by default as "externally installed"
            if (r.isEmpty())
                r = wr.setDWORD("External", 0);
        }
    } else {
        // qDebug() << "deleting " << pn;
        WindowsRegistry packages;
        r = packages.open(machineWR, keyName, KEY_ALL_ACCESS);
        if (r.isEmpty()) {
            r = packages.remove(pn);
        }
    }
    return r;
}

void InstalledPackageVersion::loadFromRegistry()
{
    WindowsRegistry entryWR;
    QString err = entryWR.open(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Npackd\\Npackd\\Packages\\" +
            this->package + "-" + this->version.getVersionString(),
            false, KEY_READ);
    if (!err.isEmpty())
        return;

    QString p = entryWR.get("Path", &err).trimmed();
    if (!err.isEmpty())
        return;

    if (p.isEmpty())
        this->directory = "";
    else {
        QDir d(p);
        if (d.exists()) {
            this->directory = p;
        } else {
            this->directory = "";
        }
    }

    this->detectionInfo = entryWR.get("DetectionInfo", &err);

    // TODO: emitStatusChanged();
}

