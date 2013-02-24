#include "abstractrepository.h"
#include "wpmutils.h"

AbstractRepository* AbstractRepository::def = 0;

AbstractRepository *AbstractRepository::getDefault_()
{
    return def;
}

void AbstractRepository::setDefault_(AbstractRepository* d)
{
    delete def;
    def = d;
}

void AbstractRepository::updateNpackdCLEnvVar()
{
    QString v = computeNpackdCLEnvVar_();

    // ignore the error for the case NPACKD_CL does not yet exist
    QString err;
    QString cur = WPMUtils::getSystemEnvVar("NPACKD_CL", &err);

    if (v != cur) {
        if (WPMUtils::setSystemEnvVar("NPACKD_CL", v).isEmpty())
            WPMUtils::fireEnvChanged();
    }
}

PackageVersion* AbstractRepository::findNewestInstalledPackageVersion_(
        const QString &name) const
{
    PackageVersion* r = 0;

    QString err; // TODO: error is not handled
    QList<PackageVersion*> pvs = this->getPackageVersions_(name, &err);
    for (int i = 0; i < pvs.count(); i++) {
        PackageVersion* p = pvs.at(i);
        if (p->installed()) {
            if (r == 0 || p->version.compare(r->version) > 0) {
                r = p;
            }
        }
    }

    if (r)
        r = r->clone();

    qDeleteAll(pvs);

    return r;
}

QString AbstractRepository::computeNpackdCLEnvVar_() const
{
    QString v;
    PackageVersion* pv;
    if (WPMUtils::is64BitWindows())
        pv = findNewestInstalledPackageVersion_(
            "com.googlecode.windows-package-manager.NpackdCL64");
    else
        pv = 0;

    if (!pv)
        pv = findNewestInstalledPackageVersion_(
            "com.googlecode.windows-package-manager.NpackdCL");

    if (pv)
        v = pv->getPath();

    delete pv;

    return v;
}

PackageVersion* AbstractRepository::findNewestInstallablePackageVersion_(
        const QString &package) const
{
    PackageVersion* r = 0;

    QString err; // TODO: the error is not handled
    QList<PackageVersion*> pvs = this->getPackageVersions_(package, &err);
    for (int i = 0; i < pvs.count(); i++) {
        PackageVersion* p = pvs.at(i);
        if (r == 0 || p->version.compare(r->version) > 0) {
            if (p->download.isValid())
                r = p;
        }
    }

    if (r)
        r = r->clone();

    qDeleteAll(pvs);

    return r;
}

AbstractRepository::AbstractRepository()
{
}

AbstractRepository::~AbstractRepository()
{
}
