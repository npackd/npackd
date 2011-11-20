#include "package.h"

Package::Package(const QString& name, const QString& title)
{
    this->name = name;
    this->title = title;
}

QString Package::getFullText()
{
    if (this->fullText.isEmpty()) {
        QString r = this->title;
        r.append(" ");
        r.append(this->description);
        r.append(" ");
        r.append(this->name);

        this->fullText = r.toLower();
    }
    return this->fullText;
}

bool Package::isValidName(QString& name)
{
    bool r = false;
    if (!name.isEmpty() && !name.contains(" ") && !name.contains("..")) {
        r = true;
    }
    return r;
}
