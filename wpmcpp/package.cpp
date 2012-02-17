#include <QUrl>

#include "package.h"
#include "xmlutils.h"

Package::Package(QObject *parent) : QObject(parent)
{
}

Package::Package(const QString& name, const QString& title)
{
    this->name = name;
    this->title = title;
}

QString Package::getFullText()
{
    QString r = this->title;
    r.append(" ");
    r.append(this->description);
    r.append(" ");
    r.append(this->name);

    return r.toLower();
}

bool Package::isValidName(QString& name)
{
    bool r = false;
    if (!name.isEmpty() && !name.contains(" ") && !name.contains("..")) {
        r = true;
    }
    return r;
}

Package* Package::createPackage(QDomElement* e, QString* err)
{
    *err = "";

    QString name = e->attribute("name").trimmed();
    if (name.isEmpty()) {
        err->append("Empty attribute 'name' in <package>)");
    }

    Package* a = new Package(name, name);

    if (err->isEmpty()) {
        a->title = XMLUtils::getTagContent(*e, "title");
        a->url = XMLUtils::getTagContent(*e, "url");
        a->description = XMLUtils::getTagContent(*e, "description");
    }

    if (err->isEmpty()) {
        a->icon = XMLUtils::getTagContent(*e, "icon");
        if (!a->icon.isEmpty()) {
            QUrl u(a->icon);
            if (!u.isValid() || u.isRelative() ||
                    !(u.scheme() == "http" || u.scheme() == "https")) {
                err->append(QString("Invalid icon URL for %1: %2").
                        arg(a->title).arg(a->icon));
            }
        }
    }

    if (err->isEmpty()) {
        a->license = XMLUtils::getTagContent(*e, "license");
    }

    if (err->isEmpty())
        return a;
    else {
        delete a;
        return 0;
    }
}

Package* Package::deserialize(const QString& xml, QString* err)
{
    QDomDocument doc;
    int errorLine, errorColumn;
    Package* p = 0;
    if (doc.setContent(xml, err, &errorLine, &errorColumn)) {
        QDomElement e = doc.documentElement();
        p = createPackage(&e, err);
    } else {
        *err = QString("XML parsing failed at line %L1, column %L2: %3").
                arg(errorLine).arg(errorColumn).arg(*err);
    }
    return p;
}

QString Package::serialize() const
{
    QDomDocument doc;
    QDomElement version = doc.createElement("package");
    doc.appendChild(version);
    saveTo(version);
    return doc.toString(4);
}

void Package::saveTo(QDomElement& e) const {
    e.setAttribute("name", name);
    XMLUtils::addTextTag(e, "title", title);
    if (!this->url.isEmpty())
        XMLUtils::addTextTag(e, "url", this->url);
    if (!description.isEmpty())
        XMLUtils::addTextTag(e, "description", description);
    if (!this->icon.isEmpty())
        XMLUtils::addTextTag(e, "icon", this->icon);
}
