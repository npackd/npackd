#include "package.h"

#include "xmlutils.h"

Package::Package(const QString& name, const QString& title)
{
    this->name = name;
    this->title = title;
}

bool Package::isValidName(QString& name)
{
    bool r = false;
    if (!name.isEmpty() && !name.contains(" ") && !name.contains("..")) {
        r = true;
    }
    return r;
}

bool Package::matches(const QStringList& terms) const
{
    bool b = true;
    if (terms.count() > 0) {
        for (int i = 0; i < terms.count(); i++) {
            const QString& term = terms.at(i);
            if (!this->title.contains(term, Qt::CaseInsensitive) &&
                    !this->description.contains(term, Qt::CaseInsensitive) &&
                    !this->name.contains(term, Qt::CaseInsensitive)
            ) {
                b = false;
                break;
            }
        }
    }
    return b;
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
    if (!this->license.isEmpty())
        XMLUtils::addTextTag(e, "license", this->license);
}
