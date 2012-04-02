#include "package.h"

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

QString Package::getFullText()
{
    if (this->fullText.isEmpty())
        this->fullText = (title + " " + description + name).toLower();
    return this->fullText;
}
