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
