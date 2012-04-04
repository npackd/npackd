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
