#include "package.h"

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
