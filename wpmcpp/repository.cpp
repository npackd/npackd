// this include should be before all the others or gcc shows errors
#include <xapian.h>

#include <windows.h>
#include <shlobj.h>

#include <QCryptographicHash>
#include "qtemporaryfile.h"
#include "downloader.h"
#include "qsettings.h"
#include "qdom.h"
#include "qdebug.h"
#include "QSet"

#include "repository.h"
#include "downloader.h"
#include "packageversionfile.h"
#include "wpmutils.h"
#include "version.h"
#include "msi.h"
#include "windowsregistry.h"
#include "xmlutils.h"
#include "packageversionhandle.h"

Repository Repository::def;

Repository::Repository(): AbstractRepository(), stemmer("english")
{
    this->db = 0;
    this->enquire = 0;
    this->queryParser = 0;
    addWellKnownPackages();

    indexer.set_stemmer(stemmer);
}

QList<PackageVersion*> Repository::findPackageVersions(
        const Xapian::Query& query) const
{
    QList<PackageVersion*> r;

    try {
        enquire->set_query(query);

        // TODO: return all results and not only the first 2000
        const unsigned int max = 2000;
        Xapian::MSet matches = enquire->get_mset(0, max);

        Xapian::MSetIterator i;
        for (i = matches.begin(); i != matches.end(); ++i) {
            Xapian::Document doc = i.get_document();
            std::string serialized = doc.get_value(2);
            QString err;
            PackageVersion* pv = PackageVersion::deserialize(
                    WPMUtils::fromUtf8StdString(serialized), &err);
            if (err.isEmpty())
                r.append(pv);
        }
    } catch (const Xapian::Error &e) {
        // TODO
    }

    return r;
}

QList<PackageVersion*> Repository::findPackageVersions(const QString& package,
        int type) const
{
    Xapian::Query query("Tpackage_version");

    if (type == 1)
        query = Xapian::Query(Xapian::Query::OP_AND, query,
                Xapian::Query("Sdetectable"));

    if (!package.isEmpty())
        query = Xapian::Query(Xapian::Query::OP_AND, query,
                Xapian::Query(Xapian::Query::OP_VALUE_RANGE, 0,
                package.toUtf8().constData(),
                package.toUtf8().constData()));

    return findPackageVersions(query);
}

QList<Package*> Repository::find(const Xapian::Query& query) const
{
    QList<Package*> r;

    if (enquire) {
        try {
            enquire->set_query(query);
            const unsigned int max = 2000;
            Xapian::MSet matches = enquire->get_mset(0, max);
            /* TODO: if (matches.size() == max)
                *warning = QString(
                        "Only the first %L1 matches of about %L2 are shown").
                        arg(max).
                        arg(matches.get_matches_estimated());*/

            Xapian::MSetIterator i;
            for (i = matches.begin(); i != matches.end(); ++i) {
                Xapian::Document doc = i.get_document();
                std::string package = doc.get_value(0);
                std::string serialized = doc.get_value(1);
                QString err;
                Package* p = Package::deserialize(
                        WPMUtils::fromUtf8StdString(serialized), &err);
                if (err.isEmpty())
                    r.append(p);
            }
        } catch (const Xapian::Error &e) {
            // TODO: *warning = WPMUtils::fromUtf8StdString(e.get_description());
        }
    }

    return r;
}

QList<Package*> Repository::find(const QString& text, int type,
        QString* warning)
{
    QString t = text.trimmed();

    Xapian::Query query("Tpackage");

    if (!t.isEmpty()) {
        query = Xapian::Query(Xapian::Query::OP_AND, query,
                queryParser->parse_query(
                t.toUtf8().constData(),
                Xapian::QueryParser::FLAG_PHRASE |
                Xapian::QueryParser::FLAG_BOOLEAN |
                Xapian::QueryParser::FLAG_LOVEHATE |
                Xapian::QueryParser::FLAG_WILDCARD |
                Xapian::QueryParser::FLAG_PARTIAL
        ));
    }

    switch (type) {
        case 1: // installed
            query = Xapian::Query(Xapian::Query::OP_AND, query,
                    Xapian::Query("Sinstalled"));
            break;
        case 2:  // installed, updateable
            query = Xapian::Query(Xapian::Query::OP_AND, query,
                    Xapian::Query("Sinstalled"));
            query = Xapian::Query(Xapian::Query::OP_AND, query,
                    Xapian::Query("Supdateable"));
            break;
    }

    return find(query);
}

QList<PackageVersion*> Repository::getInstalled()
{
    QList<PackageVersion*> ret;

    for (int i = 0; i < installedPackageVersions.count(); i++) {
        InstalledPackageVersion* ipv = installedPackageVersions.at(i);
        PackageVersion* pv = findPackageVersion(ipv->package_,
                ipv->version);
        if (pv) {
            ret.append(pv);
        }
    }

    return ret;
}

bool Repository::isLocked(const QString& package, const Version& version) const
{
    bool result = false;
    for (int i = 0; i < this->locked.count(); i++) {
        PackageVersionHandle* pvh = this->locked.at(i);
        if (pvh->package == package && pvh->version == version) {
            result = true;
            break;
        }
    }
    return result;
}

void Repository::lock(const QString& package, const Version& version)
{
    this->locked.append(new PackageVersionHandle(package, version));
}

void Repository::unlock(const QString& package, const Version& version)
{
    for (int i = 0; i < this->locked.count(); i++) {
        PackageVersionHandle* pvh = this->locked.at(i);
        if (pvh->package == package && pvh->version == version) {
            delete this->locked.takeAt(i);
            break;
        }
    }
}

Repository::~Repository()
{
    delete queryParser;
    delete enquire;
    delete db;

    qDeleteAll(this->licenses);
    qDeleteAll(this->installedPackageVersions);
    qDeleteAll(this->locked);
}

PackageVersion* Repository::findNewestInstallablePackageVersion(
        const QString &package)
{
    PackageVersion* r = 0;

    QList<PackageVersion*> list = this->getPackageVersions(package);
    for (int i = 0; i < list.count(); i++) {
        PackageVersion* p = list.at(i);
        if (r == 0 || p->version.compare(r->version) > 0) {
            if (p->download.isValid())
                r = p;
        }
    }
    return r;
}

PackageVersion* Repository::findNewestInstalledPackageVersion(
        const QString &name)
{
    PackageVersion* r = 0;

    QList<PackageVersion*> list = this->getPackageVersions(name);
    for (int i = 0; i < list.count(); i++) {
        PackageVersion* p = list.at(i);
        if (p->installed()) {
            if (r == 0 || p->version.compare(r->version) > 0) {
                r = p;
            }
        }
    }
    return r;
}

License* Repository::createLicense(QDomElement* e)
{
    QString name = e->attribute("name");
    License* a = new License(name, name);
    QDomNodeList nl = e->elementsByTagName("title");
    if (nl.count() != 0)
        a->title = nl.at(0).firstChild().nodeValue();
    nl = e->elementsByTagName("url");
    if (nl.count() != 0)
        a->url = nl.at(0).firstChild().nodeValue();
    nl = e->elementsByTagName("description");
    if (nl.count() != 0)
        a->description = nl.at(0).firstChild().nodeValue();

    return a;
}

License* Repository::findLicense(const QString& name)
{
    for (int i = 0; i < this->licenses.count(); i++) {
        if (this->licenses.at(i)->name == name)
            return this->licenses.at(i);
    }
    return 0;
}

QList<Package*> Repository::findPackages(const QString& name)
{
    Xapian::Query query("Tpackage");
    query = Xapian::Query(Xapian::Query::OP_AND, query,
            Xapian::Query(Xapian::Query::OP_VALUE_RANGE, 0,
            name.toUtf8().constData(),
            name.toUtf8().constData()));
    QList<Package*> r = find(query);

    if (name.indexOf('.') < 0) {
        query = Xapian::Query("Tpackage");
        query = Xapian::Query(Xapian::Query::OP_AND, query,
                Xapian::Query(Xapian::Query::OP_VALUE_RANGE, 2,
                name.toUtf8().constData(),
                name.toUtf8().constData()));
        r.append(find(query));
    }

    return r;
}

Package* Repository::findPackage(const QString& name) const
{
    Xapian::Query query("Tpackage");
    query = Xapian::Query(Xapian::Query::OP_AND, query,
            Xapian::Query(Xapian::Query::OP_VALUE_RANGE, 0,
            name.toUtf8().constData(),
            name.toUtf8().constData()));

    QList<Package*> ps = find(query);
    Package* p;
    if (ps.count() > 0) {
        p = ps.takeAt(0);
        qDeleteAll(ps);
        ps.clear();
    } else {
        p = 0;
    }

    // TODO: the returned object is not destroyed
    return p;
}

void Repository::indexCreateDocument(Package* p, Xapian::Document& doc)
{
    QString t = p->getFullText();
    std::string para = t.toUtf8().constData();
    doc.set_data(para);

    doc.add_value(0, p->name.toUtf8().constData());
    doc.add_value(1, p->serialize().toUtf8().constData());

    int index = p->name.indexOf('.');
    QString shortName;
    if (index > 0)
        shortName = p->name.right(p->name.length() - index - 1);
    doc.add_value(2, shortName.toUtf8().constData());

    doc.add_boolean_term("Tpackage");

    boolean installed = false, updateable = false;
    for (int i = 0; i < this->installedPackageVersions.count(); i++) {
        InstalledPackageVersion* ipv = this->installedPackageVersions.at(i);
        if (ipv->package_ == p->name) {
            installed = true;
            PackageVersion* pv = findPackageVersion(ipv->package_,
                    ipv->version);
            if (pv && pv->isUpdateEnabled()) {
                updateable = true;
                break;
            }
        }
    }
    if (installed)
        doc.add_boolean_term("Sinstalled");
    if (updateable)
        doc.add_boolean_term("Supdateable");
}

void Repository::indexCreateDocument(PackageVersion* pv, Xapian::Document& doc)
{
    QString t = pv->getFullText();
    Package* p = findPackage(pv->getPackage());
    if (p) {
        t += " ";
        t += p->getFullText();
    }

    std::string para = t.toUtf8().constData();
    doc.set_data(para);

    doc.add_value(0, pv->getPackage().toUtf8().constData());
    doc.add_value(1, pv->version.getVersionString().
            toUtf8().constData());
    doc.add_value(2, pv->serialize().toUtf8().constData());
    doc.add_value(3, pv->msiGUID.toUtf8().constData());

    doc.add_boolean_term("Tpackage_version");

    if (this->findInstalledPackageVersion(pv)) {
        doc.add_boolean_term("Sinstalled");
        if (pv->isUpdateEnabled())
            doc.add_boolean_term("Supdateable");
    } else {
        doc.add_boolean_term("Snot_installed");
    }

    if (pv->msiGUID.length() == 38) {
        doc.add_boolean_term("Aguid"); // TODO: is it used?
    }

    if (pv->msiGUID.length() == 38 || pv->detectFiles.count() > 0) {
        doc.add_boolean_term("Sdetectable");
    }
}

QString Repository::indexUpdatePackageVersion(PackageVersion* pv)
{
    QString err;
    try {
        Xapian::Query query(Xapian::Query::OP_AND,
                Xapian::Query("Tpackage_version"),
                Xapian::Query("Snot_installed"));

        enquire->set_query(query);
        Xapian::MSet matches = enquire->get_mset(0, 1);
        if (matches.size() != 0) {
            db->delete_document(*matches.begin());
        }

        Xapian::Document doc;
        this->indexCreateDocument(pv, doc);

        indexer.set_document(doc);
        indexer.index_text(doc.get_data());

        // Add the document to the database.
        db->add_document(doc);

        db->commit();
    } catch (const Xapian::Error &e) {
        err = WPMUtils::fromUtf8StdString(e.get_description());
    }
    return err;
}

void Repository::addWellKnownPackages()
{
    if (!this->findPackage("com.microsoft.Windows")) {
        Package* p = new Package("com.microsoft.Windows", "Windows");
        p->url = "http://www.microsoft.com/windows/";
        p->description = "Operating system";
        addPackage(p);
    }
    if (!this->findPackage("com.microsoft.Windows32")) {
        Package* p = new Package("com.microsoft.Windows32", "Windows/32 bit");
        p->url = "http://www.microsoft.com/windows/";
        p->description = "Operating system";
        addPackage(p);
    }
    if (!this->findPackage("com.microsoft.Windows64")) {
        Package* p = new Package("com.microsoft.Windows64", "Windows/64 bit");
        p->url = "http://www.microsoft.com/windows/";
        p->description = "Operating system";
        addPackage(p);
    }
    if (!findPackage("com.googlecode.windows-package-manager.Npackd")) {
        Package* p = new Package("com.googlecode.windows-package-manager.Npackd",
                "Npackd");
        p->url = "http://code.google.com/p/windows-package-manager/";
        p->description = "package manager";
        addPackage(p);
    }
    if (!this->findPackage("com.oracle.JRE")) {
        Package* p = new Package("com.oracle.JRE", "JRE");
        p->url = "http://www.java.com/";
        p->description = "Java runtime";
        addPackage(p);
    }
    if (!this->findPackage("com.oracle.JRE64")) {
        Package* p = new Package("com.oracle.JRE64", "JRE/64 bit");
        p->url = "http://www.java.com/";
        p->description = "Java runtime";
        addPackage(p);
    }
    if (!this->findPackage("com.oracle.JDK")) {
        Package* p = new Package("com.oracle.JDK", "JDK");
        p->url = "http://www.oracle.com/technetwork/java/javase/overview/index.html";
        p->description = "Java development kit";
        addPackage(p);
    }
    if (!this->findPackage("com.oracle.JDK64")) {
        Package* p = new Package("com.oracle.JDK64", "JDK/64 bit");
        p->url = "http://www.oracle.com/technetwork/java/javase/overview/index.html";
        p->description = "Java development kit";
        addPackage(p);
    }
    if (!this->findPackage("com.microsoft.DotNetRedistributable")) {
        Package* p = new Package("com.microsoft.DotNetRedistributable",
                ".NET redistributable runtime");
        p->url = "http://msdn.microsoft.com/en-us/netframework/default.aspx";
        p->description = ".NET runtime";
        addPackage(p);
    }
    if (!this->findPackage("com.microsoft.WindowsInstaller")) {
        Package* p = new Package("com.microsoft.WindowsInstaller",
                "Windows Installer");
        p->url = "http://msdn.microsoft.com/en-us/library/cc185688(VS.85).aspx";
        p->description = "Package manager";
        addPackage(p);
    }
    if (!this->findPackage("com.microsoft.MSXML")) {
        Package* p = new Package("com.microsoft.MSXML",
                "Microsoft Core XML Services (MSXML)");
        p->url = "http://www.microsoft.com/downloads/en/details.aspx?FamilyID=993c0bcf-3bcf-4009-be21-27e85e1857b1#Overview";
        p->description = "XML library";
        addPackage(p);
    }
}

QString Repository::planUpdates(const QList<Package*> packages,
        QList<InstallOperation*>& ops)
{
    QList<PackageVersion*> installed = getInstalled();
    QList<PackageVersion*> newest, newesti;
    QList<bool> used;

    QString err;

    for (int i = 0; i < packages.count(); i++) {
        Package* p = packages.at(i);

        PackageVersion* a = findNewestInstallablePackageVersion(p->name);
        if (a == 0) {
            err = QString("No installable version found for the package %1").
                    arg(p->title);
            break;
        }

        PackageVersion* b = findNewestInstalledPackageVersion(p->name);
        if (b == 0) {
            err = QString("No installed version found for the package %1").
                    arg(p->title);
            break;
        }

        if (a->version.compare(b->version) <= 0) {
            err = QString("The newest version (%1) for the package %2 is already installed").
                    arg(b->version.getVersionString()).arg(p->title);
            break;
        }

        newest.append(a);
        newesti.append(b);
        used.append(false);
    }

    if (err.isEmpty()) {
        // many packages cannot be installed side-by-side and overwrite for example
        // the shortcuts of the old version in the start menu. We try to find
        // those packages where the old version can be uninstalled first and then
        // the new version installed. This is the reversed order for an update.
        // If this is possible and does not affect other packages, we do this first.
        for (int i = 0; i < newest.count(); i++) {
            QList<PackageVersion*> avoid;
            QList<InstallOperation*> ops2;
            QList<PackageVersion*> installedCopy = installed;

            QString err = newesti.at(i)->planUninstallation(installedCopy, ops2);
            if (err.isEmpty()) {
                err = newest.at(i)->planInstallation(installedCopy, ops2, avoid);
                if (err.isEmpty()) {
                    if (ops2.count() == 2) {
                        used[i] = true;
                        installed = installedCopy;
                        ops.append(ops2[0]);
                        ops.append(ops2[1]);
                        ops2.clear();
                    }
                }
            }

            qDeleteAll(ops2);
        }
    }

    if (err.isEmpty()) {
        for (int i = 0; i < newest.count(); i++) {
            if (!used[i]) {
                QList<PackageVersion*> avoid;
                err = newest.at(i)->planInstallation(installed, ops, avoid);
                if (!err.isEmpty())
                    break;
            }
        }
    }

    if (err.isEmpty()) {
        for (int i = 0; i < newesti.count(); i++) {
            if (!used[i]) {
                err = newesti.at(i)->planUninstallation(installed, ops);
                if (!err.isEmpty())
                    break;
            }
        }
    }

    if (err.isEmpty()) {
        InstallOperation::simplify(ops);
    }

    return err;
}

void Repository::detectWindows()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    Version v;
    v.setVersion(osvi.dwMajorVersion, osvi.dwMinorVersion,
            osvi.dwBuildNumber);

    clearExternallyInstalled("com.microsoft.Windows");
    clearExternallyInstalled("com.microsoft.Windows32");
    clearExternallyInstalled("com.microsoft.Windows64");

    addInstalledPackageVersionIfAbsent("com.microsoft.Windows", v,
            WPMUtils::getWindowsDir(), true);
    if (WPMUtils::is64BitWindows()) {
        addInstalledPackageVersionIfAbsent("com.microsoft.Windows64", v,
                WPMUtils::getWindowsDir(), true);
    } else {
        addInstalledPackageVersionIfAbsent("com.microsoft.Windows32", v,
                WPMUtils::getWindowsDir(), true);
    }
}

void Repository::recognize(Job* job)
{
    job->setProgress(0);

    if (!job->isCancelled()) {
        job->setHint("Detecting Windows");
        detectWindows();
        job->setProgress(0.1);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting JRE");
        detectJRE(false);
        if (WPMUtils::is64BitWindows())
            detectJRE(true);
        job->setProgress(0.4);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting JDK");
        detectJDK(false);
        if (WPMUtils::is64BitWindows())
            detectJDK(true);
        job->setProgress(0.7);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting .NET");
        detectDotNet();
        job->setProgress(0.8);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting MSI packages");
        detectMSIProducts();
        job->setProgress(0.9);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting Windows Installer");
        detectMicrosoftInstaller();
        job->setProgress(0.95);
    }

    if (!job->isCancelled()) {
        job->setHint("Detecting Microsoft Core XML Services (MSXML)");
        detectMSXML();
        job->setProgress(0.97);
    }

    if (!job->isCancelled()) {
        job->setHint("Updating NPACKD_CL");
        updateNpackdCLEnvVar();
        job->setProgress(1);
    }

    job->complete();
}

void Repository::detectJRE(bool w64bit)
{
    clearExternallyInstalled(w64bit ? "com.oracle.JRE64" : "com.oracle.JRE");

    if (w64bit && !WPMUtils::is64BitWindows())
        return;

    WindowsRegistry jreWR;
    QString err = jreWR.open(HKEY_LOCAL_MACHINE,
            "Software\\JavaSoft\\Java Runtime Environment", !w64bit, KEY_READ);
    if (err.isEmpty()) {
        QStringList entries = jreWR.list(&err);
        for (int i = 0; i < entries.count(); i++) {
            QString v_ = entries.at(i);
            v_ = v_.replace('_', '.');
            Version v;
            if (!v.setVersion(v_) || v.getNParts() <= 2)
                continue;

            WindowsRegistry wr;
            err = wr.open(jreWR, entries.at(i), KEY_READ);
            if (!err.isEmpty())
                continue;

            QString path = wr.get("JavaHome", &err);
            if (!err.isEmpty())
                continue;

            QDir d(path);
            if (!d.exists())
                continue;

            addInstalledPackageVersionIfAbsent(w64bit ? "com.oracle.JRE64" :
                    "com.oracle.JRE", v, path, true);
        }
    }
}

void Repository::detectJDK(bool w64bit)
{
    QString p = w64bit ? "com.oracle.JDK64" : "com.oracle.JDK";

    clearExternallyInstalled(p);

    if (w64bit && !WPMUtils::is64BitWindows())
        return;

    WindowsRegistry wr;
    QString err = wr.open(HKEY_LOCAL_MACHINE,
            "Software\\JavaSoft\\Java Development Kit",
            !w64bit, KEY_READ);
    if (err.isEmpty()) {
        QStringList entries = wr.list(&err);
        if (err.isEmpty()) {
            for (int i = 0; i < entries.count(); i++) {
                QString v_ = entries.at(i);
                WindowsRegistry r;
                err = r.open(wr, v_, KEY_READ);
                if (!err.isEmpty())
                    continue;

                v_.replace('_', '.');
                Version v;
                if (!v.setVersion(v_) || v.getNParts() <= 2)
                    continue;

                QString path = r.get("JavaHome", &err);
                if (!err.isEmpty())
                    continue;

                QDir d(path);
                if (!d.exists())
                    continue;

                addInstalledPackageVersionIfAbsent(p, v, path, true);
            }
        }
    }
}

void Repository::clearExternallyInstalled(QString package)
{
    for (int i = 0; i < this->installedPackageVersions.count();) {
        InstalledPackageVersion* ipv = this->installedPackageVersions.at(i);
        if (ipv->package_ == package && ipv->external_) {
            this->installedPackageVersions.removeAt(i);
            delete ipv;
        } else {
            i++;
        }
    }
}

void Repository::addInstalledPackageVersionIfAbsent(const QString& package,
        const Version& version, const QString& ipath, bool external)
{
    InstalledPackageVersion* ipv = findInstalledPackageVersion(
            package, version);
    if (!ipv) {
        ipv = new InstalledPackageVersion(package, version, ipath, external);
        this->installedPackageVersions.append(ipv);
    }
    ipv->ipath = ipath;
    ipv->external_ = external;
}

void Repository::removeInstalledPackageVersion(PackageVersion* pv)
{
    InstalledPackageVersion* ipv = findInstalledPackageVersion(pv);
    if (ipv) {
        this->installedPackageVersions.removeOne(ipv);
        delete ipv;
    }
}

void Repository::detectOneDotNet(const WindowsRegistry& wr,
        const QString& keyName)
{
    QString packageName("com.microsoft.DotNetRedistributable");
    Version keyVersion;

    Version oneOne(1, 1);
    Version four(4, 0);
    Version two(2, 0);

    Version v;
    bool found = false;
    if (keyName.startsWith("v") && keyVersion.setVersion(
            keyName.right(keyName.length() - 1))) {
        if (keyVersion.compare(oneOne) < 0) {
            // not yet implemented
        } else if (keyVersion.compare(two) < 0) {
            v = keyVersion;
            found = true;
        } else if (keyVersion.compare(four) < 0) {
            QString err;
            QString value_ = wr.get("Version", &err);
            if (err.isEmpty() && v.setVersion(value_)) {
                found = true;
            }
        } else {
            WindowsRegistry r;
            QString err = r.open(wr, "Full", KEY_READ);
            if (err.isEmpty()) {
                QString value_ = r.get("Version", &err);
                if (err.isEmpty() && v.setVersion(value_)) {
                    found = true;
                }
            }
        }
    }

    if (found) {
        addInstalledPackageVersionIfAbsent(packageName, v,
                WPMUtils::getWindowsDir(), true);
    }
}

void Repository::detectMSIProducts()
{
    QStringList all = WPMUtils::findInstalledMSIProducts();
    // qDebug() << all.at(0);

    for (int i = 0; i < all.count(); i++) {
        QString guid = all.at(i);
        Xapian::Query query("Tpackage_version");

        query = Xapian::Query(Xapian::Query::OP_AND, query,
                Xapian::Query(Xapian::Query::OP_VALUE_RANGE, 3,
                guid.toUtf8().constData(),
                guid.toUtf8().constData()));

        QList<PackageVersion*> pvs = findPackageVersions(query);
        if (pvs.count() > 0) {
            PackageVersion* pv = pvs.at(0);
            InstalledPackageVersion* ipv = findInstalledPackageVersion(
                    pv->package_, pv->version);
            if (!ipv) {
                QString err;
                QString location = WPMUtils::getMSIProductLocation(
                        all.at(i), &err);
                if (location.isEmpty() || !err.isEmpty())
                    location = WPMUtils::getWindowsDir();

                if (!ipv) {
                    Package* p = findPackage(pv->package_);

                    // this should always be true
                    if (p) {
                        ipv = new InstalledPackageVersion(p->name,
                                pv->version, location, true);
                        this->installedPackageVersions.append(ipv);
                    }
                }
            }
        }
        qDeleteAll(pvs);
        pvs.clear();
    }
}

void Repository::detectDotNet()
{
    // http://stackoverflow.com/questions/199080/how-to-detect-what-net-framework-versions-and-service-packs-are-installed

    clearExternallyInstalled("com.microsoft.DotNetRedistributable");

    WindowsRegistry wr;
    QString err = wr.open(HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\NET Framework Setup\\NDP", false, KEY_READ);
    if (err.isEmpty()) {
        QStringList entries = wr.list(&err);
        if (err.isEmpty()) {
            for (int i = 0; i < entries.count(); i++) {
                QString v_ = entries.at(i);
                Version v;
                if (v_.startsWith("v") && v.setVersion(
                        v_.right(v_.length() - 1))) {
                    WindowsRegistry r;
                    err = r.open(wr, v_, KEY_READ);
                    if (err.isEmpty())
                        detectOneDotNet(r, v_);
                }
            }
        }
    }
}

void Repository::detectMicrosoftInstaller()
{
    clearExternallyInstalled("com.microsoft.WindowsInstaller");

    Version v = WPMUtils::getDLLVersion("MSI.dll");
    Version nullNull(0, 0);
    if (v.compare(nullNull) > 0) {
        addInstalledPackageVersionIfAbsent("com.microsoft.WindowsInstaller", v,
                WPMUtils::getWindowsDir(), true);
    }
}

void Repository::detectMSXML()
{
    clearExternallyInstalled("com.microsoft.MSXML");

    Version v = WPMUtils::getDLLVersion("msxml.dll");
    Version nullNull(0, 0);
    if (v.compare(nullNull) > 0) {
        addInstalledPackageVersionIfAbsent("com.microsoft.MSXML", v,
                WPMUtils::getWindowsDir(), true);
    }
    v = WPMUtils::getDLLVersion("msxml2.dll");
    if (v.compare(nullNull) > 0) {
        addInstalledPackageVersionIfAbsent("com.microsoft.MSXML", v,
                WPMUtils::getWindowsDir(), true);
    }
    v = WPMUtils::getDLLVersion("msxml3.dll");
    if (v.compare(nullNull) > 0) {
        v.prepend(3);
        addInstalledPackageVersionIfAbsent("com.microsoft.MSXML", v,
                WPMUtils::getWindowsDir(), true);
    }
    v = WPMUtils::getDLLVersion("msxml4.dll");
    if (v.compare(nullNull) > 0) {
        addInstalledPackageVersionIfAbsent("com.microsoft.MSXML", v,
                WPMUtils::getWindowsDir(), true);
    }
    v = WPMUtils::getDLLVersion("msxml5.dll");
    if (v.compare(nullNull) > 0) {
        addInstalledPackageVersionIfAbsent("com.microsoft.MSXML", v,
                WPMUtils::getWindowsDir(), true);
    }
    v = WPMUtils::getDLLVersion("msxml6.dll");
    if (v.compare(nullNull) > 0) {
        addInstalledPackageVersionIfAbsent("com.microsoft.MSXML", v,
                WPMUtils::getWindowsDir(), true);
    }
}

PackageVersion* Repository::findPackageVersion(const QString& package,
        const Version& version) const
{
    PackageVersion* r = 0;

    QList<PackageVersion*> list = this->getPackageVersions(package);
    for (int i = 0; i < list.count(); i++) {
        PackageVersion* p = list.at(i);
        if (p->version.compare(version) == 0) {
            r = p;
            list.removeAt(i);
            break;
        }
    }
    qDeleteAll(list);

    // TODO: returned object is never destroyed
    return r;
}

/* TODO
QString Repository::writeTo(const QString& filename) const
{
    QString r;

    QDomDocument doc;
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    XMLUtils::addTextTag(root, "spec-version", "3");

    for (int i = 0; i < getPackageCount(); i++) {
        Package* p = packages.at(i);
        QDomElement package = doc.createElement("package");
        p->saveTo(package);
        root.appendChild(package);
    }

    for (int i = 0; i < getPackageVersionCount(); i++) {
        PackageVersion* pv = getPackageVersion(i);
        QDomElement version = doc.createElement("version");
        version.setAttribute("name", pv->version.getVersionString());
        version.setAttribute("package", pv->getPackage()->name);
        if (pv->download.isValid())
            XMLUtils::addTextTag(version, "url", pv->download.toString());
        root.appendChild(version);
    }

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream s(&file);
        doc.save(s, 4);
    } else {
        r = QString("Cannot open %1 for writing").arg(filename);
    }

    return "";
}
*/

void Repository::process(Job *job, const QList<InstallOperation *> &install)
{
    for (int j = 0; j < install.size(); j++) {
        InstallOperation* op = install.at(j);
        PackageVersion* pv = op->packageVersion;
        pv->lock();
    }

    int n = install.count();

    for (int i = 0; i < install.count(); i++) {
        InstallOperation* op = install.at(i);
        PackageVersion* pv = op->packageVersion;
        if (op->install)
            job->setHint(QString("Installing %1").arg(
                    pv->toString()));
        else
            job->setHint(QString("Uninstalling %1").arg(
                    pv->toString()));
        Job* sub = job->newSubJob(1.0 / n);
        if (op->install)
            pv->install(sub, pv->getPreferredInstallationDirectory());
        else
            pv->uninstall(sub);
        if (!sub->getErrorMessage().isEmpty())
            job->setErrorMessage(sub->getErrorMessage());
        delete sub;

        if (!job->getErrorMessage().isEmpty())
            break;
    }

    for (int j = 0; j < install.size(); j++) {
        InstallOperation* op = install.at(j);
        PackageVersion* pv = op->packageVersion;
        pv->unlock();
    }

    job->complete();
}

void Repository::scanPre1_15Dir(bool exact)
{
    QDir aDir(WPMUtils::getInstallationDirectory());
    if (!aDir.exists())
        return;

    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false);
    QString err;
    WindowsRegistry packagesWR = machineWR.createSubKey(
            "SOFTWARE\\Npackd\\Npackd\\Packages", &err);
    if (!err.isEmpty())
        return;

    QFileInfoList entries = aDir.entryInfoList(
            QDir::NoDotAndDotDot | QDir::Dirs);
    int count = entries.size();
    QString dirPath = aDir.absolutePath();
    dirPath.replace('/', '\\');
    for (int idx = 0; idx < count; idx++) {
        QFileInfo entryInfo = entries[idx];
        QString name = entryInfo.fileName();
        int pos = name.lastIndexOf("-");
        if (pos > 0) {
            QString packageName = name.left(pos);
            QString versionName = name.right(name.length() - pos - 1);

            if (Package::isValidName(packageName)) {
                Version version;
                if (version.setVersion(versionName)) {
                    if (!exact || this->findPackage(packageName)) {
                        // using getVersionString() here to fix a bug in earlier
                        // versions where version numbers were not normalized
                        WindowsRegistry wr = packagesWR.createSubKey(
                                packageName + "-" + version.getVersionString(),
                                &err);
                        if (err.isEmpty()) {
                            wr.set("Path", dirPath + "\\" +
                                    name);
                            wr.setDWORD("External", 0);
                        }
                    }
                }
            }
        }
    }
}

QString Repository::computeNpackdCLEnvVar()
{
    QString v;
    PackageVersion* pv = findNewestInstalledPackageVersion(
            "com.googlecode.windows-package-manager.NpackdCL");
    if (pv) {
        InstalledPackageVersion* ipv = findInstalledPackageVersion(pv);
        v = ipv->ipath;
    }

    return v;
}

void Repository::updateNpackdCLEnvVar()
{
    QString v = computeNpackdCLEnvVar();

    // ignore the error for the case NPACKD_CL does not yet exist
    QString err;
    QString cur = WPMUtils::getSystemEnvVar("NPACKD_CL", &err);

    if (v != cur) {
        if (WPMUtils::setSystemEnvVar("NPACKD_CL", v).isEmpty())
            WPMUtils::fireEnvChanged();
    }
}

void Repository::detectPre_1_15_Packages()
{
    QString regPath = "SOFTWARE\\Npackd\\Npackd";
    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false);
    QString err;
    WindowsRegistry npackdWR = machineWR.createSubKey(regPath, &err);
    if (err.isEmpty()) {
        DWORD b = npackdWR.getDWORD("Pre1_15DirScanned", &err);
        if (!err.isEmpty() || b != 1) {
            // store the references to packages in the old format (< 1.15)
            // in the registry
            scanPre1_15Dir(false);
            npackdWR.setDWORD("Pre1_15DirScanned", 1);
        }
    }
}

void Repository::readRegistryDatabase()
{
    qDeleteAll(this->installedPackageVersions);
    this->installedPackageVersions.clear();

    WindowsRegistry machineWR(HKEY_LOCAL_MACHINE, false, KEY_READ);

    QString err;
    WindowsRegistry packagesWR;
    err = packagesWR.open(machineWR,
            "SOFTWARE\\Npackd\\Npackd\\Packages", KEY_READ);
    if (err.isEmpty()) {
        QStringList entries = packagesWR.list(&err);
        for (int i = 0; i < entries.count(); ++i) {
            QString name = entries.at(i);
            int pos = name.lastIndexOf("-");
            if (pos > 0) {
                QString packageName = name.left(pos);
                if (Package::isValidName(packageName)) {
                    QString versionName = name.right(name.length() - pos - 1);
                    Version version;
                    if (version.setVersion(versionName)) {
                        loadInstallationInfoFromRegistry(packageName,
                                version);
                    }
                }
            }
        }
    }
}

void Repository::loadInstallationInfoFromRegistry(const QString& package,
        const Version& version)
{
    WindowsRegistry entryWR;
    QString err = entryWR.open(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Npackd\\Npackd\\Packages\\" +
            package + "-" + version.getVersionString(),
            false, KEY_READ);
    if (!err.isEmpty())
        return;

    QString p = entryWR.get("Path", &err).trimmed();
    if (!err.isEmpty())
        return;

    DWORD external = entryWR.getDWORD("External", &err);
    if (!err.isEmpty())
        external = 1;

    QString ipath;
    if (p.isEmpty())
        ipath = "";
    else {
        QDir d(p);
        if (d.exists()) {
            ipath = p;
        } else {
            ipath = "";
        }
    }

    if (!ipath.isEmpty()) {
        InstalledPackageVersion* ipv = new InstalledPackageVersion(package,
                version, ipath, external != 0);
        this->installedPackageVersions.append(ipv);
    }

    // TODO: emitStatusChanged();
}

InstalledPackageVersion* Repository::findInstalledPackageVersion(
        const PackageVersion* pv)
{
    InstalledPackageVersion* r = 0;
    for (int i = 0; i < this->installedPackageVersions.count(); i++) {
        InstalledPackageVersion* ipv = this->installedPackageVersions.at(i);
        if (ipv->package_ == pv->package_ && ipv->version == pv->version) {
            r = ipv;
            break;
        }
    }
    return r;
}

InstalledPackageVersion* Repository::findInstalledPackageVersion(
        const QString& package, const Version& version) const
{
    InstalledPackageVersion* r = 0;
    for (int i = 0; i < this->installedPackageVersions.count(); i++) {
        InstalledPackageVersion* ipv = this->installedPackageVersions.at(i);
        if (ipv->package_ == package && ipv->version == version) {
            r = ipv;
            break;
        }
    }
    return r;
}

void Repository::scan(const QString& path, Job* job, int level,
        QStringList& ignore)
{
    if (ignore.contains(path))
        return;

    QDir aDir(path);

    QMap<QString, QString> path2sha1;

    QList<PackageVersion*> pvs = this->findPackageVersions("", 1);

    for (int i = 0; i < pvs.count(); i++) {
        if (job && job->isCancelled())
            break;

        PackageVersion* pv = pvs.at(i);
        InstalledPackageVersion* ipv = this->findInstalledPackageVersion(pv);
        if (!ipv && pv->detectFiles.count() > 0) {
            boolean ok = true;
            for (int j = 0; j < pv->detectFiles.count(); j++) {
                bool fileOK = false;
                DetectFile* df = pv->detectFiles.at(j);
                if (aDir.exists(df->path)) {
                    QString fullPath = path + "\\" + df->path;
                    QFileInfo f(fullPath);
                    if (f.isFile() && f.isReadable()) {
                        QString sha1 = path2sha1[df->path];
                        if (sha1.isEmpty()) {
                            sha1 = WPMUtils::sha1(fullPath);
                            path2sha1[df->path] = sha1;
                        }
                        if (df->sha1 == sha1) {
                            fileOK = true;
                        }
                    }
                }
                if (!fileOK) {
                    ok = false;
                    break;
                }
            }

            if (ok) {
                ipv = new InstalledPackageVersion(pv->package_, pv->version,
                        path, true);
                this->installedPackageVersions.append(ipv);
                return;
            }
        }
    }

    if (job && !job->isCancelled()) {
        QFileInfoList entries = aDir.entryInfoList(
                QDir::NoDotAndDotDot | QDir::Dirs);
        int count = entries.size();
        for (int idx = 0; idx < count; idx++) {
            if (job && job->isCancelled())
                break;

            QFileInfo entryInfo = entries[idx];
            QString name = entryInfo.fileName();

            if (job) {
                job->setHint(QString("%1").arg(name));
                if (job->isCancelled())
                    break;
            }

            Job* djob;
            if (level < 2)
                djob = job->newSubJob(1.0 / count);
            else
                djob = 0;
            scan(path + "\\" + name.toLower(), djob, level + 1, ignore);
            delete djob;

            if (job) {
                job->setProgress(((double) idx) / count);
            }
        }
    }

    qDeleteAll(pvs);
    pvs.clear();

    if (job)
        job->complete();
}

void Repository::scanHardDrive(Job* job)
{
    QStringList ignore;
    ignore.append(WPMUtils::normalizePath(WPMUtils::getWindowsDir()));

    QFileInfoList fil = QDir::drives();
    for (int i = 0; i < fil.count(); i++) {
        if (job->isCancelled())
            break;

        QFileInfo fi = fil.at(i);

        job->setHint(QString("Scanning %1").arg(fi.absolutePath()));
        Job* djob = job->newSubJob(1.0 / fil.count());
        QString path = WPMUtils::normalizePath(fi.absolutePath());
        UINT t = GetDriveType((WCHAR*) path.utf16());
        if (t == DRIVE_FIXED)
            scan(path, djob, 0, ignore);
        delete djob;
    }

    job->complete();
}

void Repository::reload(Job *job)
{
    /* debugging
    QList<PackageVersion*> msw = this->getPackageVersions(
            "com.microsoft.Windows");
    qDebug() << "Repository::recognize " << msw.count();
    for (int i = 0; i < msw.count(); i++) {
        qDebug() << msw.at(i)->toString() << " " << msw.at(i)->getStatus();
    }
    */

    job->setHint("Loading repositories");

    QList<QUrl*> urls = getRepositoryURLs();
    QList<QTemporaryFile*> files;
    QString key;
    if (urls.count() > 0) {
        for (int i = 0; i < urls.count(); i++) {
            job->setHint(QString("Downloading repository %1 of %2").arg(i + 1).
                    arg(urls.count()));
            Job* s = job->newSubJob(0.5 / urls.count());
            QString sha1;
            QTemporaryFile* f = Downloader::download(s,
                    *urls.at(i), &sha1, true);
            if (!s->getErrorMessage().isEmpty()) {
                job->setErrorMessage(QString(
                        "Error downloading the repository %1: %2").arg(
                        urls.at(i)->toString()).arg(s->getErrorMessage()));
                delete s;
                break;
            }
            delete s;

            key += sha1;
            files.append(f);

            if (job->isCancelled())
                break;
        }
    } else {
        job->setErrorMessage("No repositories defined");
        job->setProgress(0.5);
    }

    key.append("6"); // serialization version

    // TODO: can we apply SHA1 to itself?
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(key.toAscii());
    key = hash.result().toHex().toLower();

    bool indexed = false;
    WindowsRegistry wr;
    QString err = wr.open(HKEY_LOCAL_MACHINE,
            "Software\\Npackd\\Npackd\\Index", false,
            KEY_READ);
    if (err.isEmpty()) {
        QString storedKey = wr.get("SHA1", &err);
        if (err.isEmpty() && key == storedKey) {
            indexed = true;
        }
    }

    // qDebug() << "Repository::load.3";

    job->complete();

    QString fn;
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Creating index directory");

        // Open the database for update, creating a new database if necessary.
        fn = WPMUtils::getShellDir(CSIDL_LOCAL_APPDATA) +
                "\\Npackd\\Npackd";
        QDir dir(fn);
        if (!dir.exists(fn)) {
            if (!dir.mkpath(fn)) {
                job->setErrorMessage(
                        QString("Cannot create the directory %1").arg(fn));
            }
        }

        job->setProgress(0.65);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        delete this->enquire;
        delete this->queryParser;
        delete db;

        QString indexDir = fn + "\\Index";
        QDir d(indexDir);

        if (!d.exists())
            indexed = false;

        try {
            if (!indexed) {
                db = new Xapian::WritableDatabase(
                        indexDir.toUtf8().constData(),
                        Xapian::DB_CREATE_OR_OVERWRITE);

                WindowsRegistry wr;
                QString err = wr.open(HKEY_LOCAL_MACHINE,
                        "Software", false, KEY_ALL_ACCESS);
                if (err.isEmpty()) {
                    WindowsRegistry indexReg = wr.createSubKey(
                            "Npackd\\Npackd\\Index", &err, KEY_ALL_ACCESS);
                    if (err.isEmpty()) {
                        indexReg.set("SHA1", key);
                    }
                }
            } else {
                db = new Xapian::WritableDatabase(
                        indexDir.toUtf8().constData(),
                        Xapian::DB_CREATE_OR_OPEN);
                job->setProgress(1);
            }

            this->enquire = new Xapian::Enquire(*this->db);
            this->queryParser = new Xapian::QueryParser();
            this->queryParser->set_database(*this->db);
            queryParser->set_stemmer(stemmer);
            queryParser->set_default_op(Xapian::Query::OP_AND);
        } catch (const Xapian::Error &e) {
            job->setErrorMessage(WPMUtils::fromUtf8StdString(
                    e.get_description()));
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        for (int i = 0; i < urls.count(); i++) {
            job->setHint(QString("Processing repository %1 of %2").arg(i + 1).
                    arg(urls.count()));
            Job* s = job->newSubJob(0.2 / urls.count());
            loadOne(files.at(i), s, !indexed);
            if (!s->getErrorMessage().isEmpty()) {
                job->setErrorMessage(QString(
                        "Error processing the repository %1: %2").arg(
                        urls.at(i)->toString()).arg(s->getErrorMessage()));
                delete s;
                break;
            }
            delete s;

            if (job->isCancelled())
                break;
        }
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        addWellKnownPackages();
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        Job* d = job->newSubJob(0.1);
        job->setHint("Refreshing installation statuses");
        refresh(d);
        delete d;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        if (!indexed) {
            // Explicitly commit so that we get to see any errors.  WritableDatabase's
            // destructor will commit implicitly (unless we're in a transaction) but
            // will swallow any exceptions produced.
            job->setHint("preparing the index");
            db->commit();
        }
    }

    // TODO: check setProgress in the whole method

    qDeleteAll(urls);
    qDeleteAll(files);

    job->complete();

    /* debugging
    msw = this->getPackageVersions(
            "com.microsoft.Windows");
    qDebug() << "Repository::recognize 2 " << msw.count();
    for (int i = 0; i < msw.count(); i++) {
        qDebug() << msw.at(i)->toString() << " " << msw.at(i)->getStatus();
    }
    */
}

void Repository::refresh(Job *job)
{
    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting packages installed by Npackd 1.14 or earlier");
        this->detectPre_1_15_Packages();
        job->setProgress(0.4);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Reading registry package database");
        this->readRegistryDatabase();
        job->setProgress(0.5);
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting software");
        Job* d = job->newSubJob(0.2);
        this->recognize(d);
        delete d;
    }

    if (!job->isCancelled() && job->getErrorMessage().isEmpty()) {
        job->setHint("Detecting packages installed by Npackd 1.14 or earlier (2)");
        scanPre1_15Dir(true);
        job->setProgress(1);
    }

    job->complete();
}

void Repository::loadOne(QTemporaryFile* f, Job* job, bool index) {
    job->setHint("Downloading");

    QDomDocument doc;
    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        job->setHint("Parsing the content");
        // qDebug() << "Repository::loadOne.2";
        int errorLine;
        int errorColumn;
        QString errMsg;
        if (!doc.setContent(f, &errMsg, &errorLine, &errorColumn))
            job->setErrorMessage(QString(
                    "XML parsing failed at line %L1, column %L2: %3").
                    arg(errorLine).arg(errorColumn).arg(errMsg));
        else
            job->setProgress(0.91);
    }

    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        Job* djob = job->newSubJob(0.09);
        loadOne(&doc, djob, index);
        if (!djob->getErrorMessage().isEmpty())
            job->setErrorMessage(QString("Error loading XML: %2").
                    arg(djob->getErrorMessage()));
        delete djob;
    }

    job->complete();
}

void Repository::addPackage(Package* p) {
    // TODO: this->packages.append(p);
}

QList<PackageVersion*> Repository::getPackageVersions(QString package) const {
    QList<PackageVersion*> list = this->findPackageVersions(package, 0);
    // TODO: results are not sorted qSort(list.begin(), list.end());

    // TODO: results are not destroyed
    return list;
}

QList<Version> Repository::getPackageVersions2(QString package) const {
    QList<PackageVersion*> pvs = findPackageVersions(package, 0);
    QList<Version> list;
    for (int i = 0; i < pvs.count(); i++) {
        list.append(pvs.at(i)->version);
    }
    qDeleteAll(pvs);
    pvs.clear();
    qSort(list.begin(), list.end());
    return list;
}

QList<Version> Repository::getInstalledPackageVersions(QString package) const {
    QList<Version> list;
    for (int i = 0; i < this->installedPackageVersions.count(); i++) {
        InstalledPackageVersion* ipv = this->installedPackageVersions.at(i);
        if (ipv->package_ == package) {
            list.append(ipv->version);
        }
    }
    qSort(list.begin(), list.end());
    return list;
}

void Repository::loadOne(QDomDocument* doc, Job* job, bool index)
{
    QDomElement root;
    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        root = doc->documentElement();
        QDomNodeList nl = root.elementsByTagName("spec-version");
        if (nl.count() != 0) {
            QString specVersion = nl.at(0).firstChild().nodeValue();
            Version specVersion_;
            if (!specVersion_.setVersion(specVersion)) {
                job->setErrorMessage(QString(
                        "Invalid repository specification version: %1").
                        arg(specVersion));
            } else {
                if (specVersion_.compare(Version(4, 0)) >= 0)
                    job->setErrorMessage(QString(
                            "Incompatible repository specification version: %1. \n"
                            "Plese download a newer version of Npackd from http://code.google.com/p/windows-package-manager/").
                            arg(specVersion));
                else
                    job->setProgress(0.01);
            }
        } else {
            job->setProgress(0.01);
        }
    }

    if (job->getErrorMessage().isEmpty() && !job->isCancelled()) {
        for (QDomNode n = root.firstChild(); !n.isNull();
                n = n.nextSibling()) {
            if (n.isElement()) {
                QDomElement e = n.toElement();
                if (e.nodeName() == "license") {
                    License* p = createLicense(&e);
                    if (this->findLicense(p->name))
                        delete p;
                    else
                        this->licenses.append(p);
                }
            }
        }

        try {
            for (QDomNode n = root.firstChild(); !n.isNull();
                    n = n.nextSibling()) {
                if (n.isElement()) {
                    QDomElement e = n.toElement();
                    if (e.nodeName() == "package") {
                        QString err;
                        Package* p = Package::createPackage(&e, &err);
                        if (p) {
                            if (this->findPackage(p->name))
                                delete p;
                            else {
                                this->addPackage(p);
                                if (index) {
                                    // qDebug() << p->name;

                                    Xapian::Document doc;
                                    this->indexCreateDocument(p, doc);

                                    indexer.set_document(doc);
                                    indexer.index_text(doc.get_data());

                                    // Add the document to the database.
                                    db->add_document(doc);
                                }
                            }
                        } else {
                            job->setErrorMessage(err);
                            break;
                        }
                    }
                }
            }
            for (QDomNode n = root.firstChild(); !n.isNull();
                    n = n.nextSibling()) {
                if (n.isElement()) {
                    QDomElement e = n.toElement();
                    if (e.nodeName() == "version") {
                        QString err;
                        PackageVersion* pv = PackageVersion::createPackageVersion(
                                &e, &err);
                        if (pv) {
                            /* TODO: if (!this->findPackageVersion(pv->getPackage(),
                                    pv->version)) {*/
                                if (index) {
                                    Xapian::Document doc;
                                    this->indexCreateDocument(pv, doc);

                                    indexer.set_document(doc);

                                    // TODO: do we need this?
                                    indexer.index_text(doc.get_data());

                                    // Add the document to the database.
                                    db->add_document(doc);

                                    // Debugging:
                                    // qDebug() << pv->package_ << " " <<
                                    // pv->version.getVersionString();
                                }
                            /* TODO }*/
                            delete pv;
                        } else {
                            job->setErrorMessage(err);
                            break;
                        }
                    }
                }
            }
        } catch (const Xapian::Error &e) {
            job->setErrorMessage(WPMUtils::fromUtf8StdString(
                    e.get_description()));
        }
        job->setProgress(1);
    }

    // TODO: check setProgress in the whole method

    job->complete();
}

void Repository::fireStatusChanged(PackageVersion *pv)
{
    emit statusChanged(pv);
}

PackageVersion* Repository::findLockedPackageVersion() const
{
    PackageVersion* r = 0;
    if (locked.count() > 0) {
        PackageVersionHandle* pvh = locked.at(0);
        r = findPackageVersion(pvh->package, pvh->version);
    }
    return r;
}

QList<QUrl*> Repository::getRepositoryURLs()
{
    QList<QUrl*> r;

    WindowsRegistry wr;
    if (wr.open(HKEY_LOCAL_MACHINE,
            "Software\\Npackd\\Npackd\\Reps", false, KEY_READ).isEmpty()) {
        QString err;
        int count = wr.getDWORD("Count", &err);
        if (err.isEmpty()) {
            for (int i = 0; i < count; i++) {
                WindowsRegistry wr2;
                if (wr2.open(wr, QString("%1").arg(i)).isEmpty()) {
                    QString url = wr2.get("URL", &err);
                    if (err.isEmpty()) {
                        r.append(new QUrl(url));
                    }
                }
            }
        }
    } else {
        if (r.size() == 0) {
            QSettings s1("Npackd", "Npackd");
            int size = s1.beginReadArray("repositories");
            for (int i = 0; i < size; ++i) {
                s1.setArrayIndex(i);
                QString v = s1.value("repository").toString();
                r.append(new QUrl(v));
            }
            s1.endArray();
        }

        if (r.size() == 0) {
            QSettings s("WPM", "Windows Package Manager");

            int size = s.beginReadArray("repositories");
            for (int i = 0; i < size; ++i) {
                s.setArrayIndex(i);
                QString v = s.value("repository").toString();
                r.append(new QUrl(v));
            }
            s.endArray();

            if (size == 0) {
                QString v = s.value("repository", "").toString();
                if (v != "") {
                    r.append(new QUrl(v));
                }
            }
        }

        setRepositoryURLs(r);
    }
    
    return r;
}

void Repository::setRepositoryURLs(QList<QUrl*>& urls)
{
    WindowsRegistry wr;
    if (wr.open(HKEY_LOCAL_MACHINE, "Software", false).isEmpty()) {
        QString err;
        WindowsRegistry wr2 = wr.createSubKey("Npackd\\Npackd\\Reps", &err);
        if (err.isEmpty()) {
            err = wr2.setDWORD("Count", urls.count());
            if (err.isEmpty()) {
                for (int i = 0; i < urls.count(); i++) {
                    WindowsRegistry wr3 = wr2.createSubKey(
                            QString("%1").arg(i), &err);
                    if (err.isEmpty()) {
                        wr3.set("URL", urls.at(i)->toString());
                    }
                }
            }
        }
    }
}

Repository* Repository::getDefault()
{
    return &def;
}
