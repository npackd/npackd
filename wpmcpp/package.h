#ifndef PACKAGE_H
#define PACKAGE_H

#include <QString>
#include <QStringList>

/**
 * A package declaration.
 */
class Package
{
private:
    QString fullText;
public:
    /** name of the package like "org.buggysoft.BuggyEditor" */
    QString name;

    /** short program title (1 line) like "Gimp" */
    QString title;

    /** web page associated with this piece of software. May be empty. */
    QString url;

    /** icon URL or "" */
    QString icon;

    /** description. May contain multiple lines and paragraphs. */
    QString description;

    /** name of the license like "org.gnu.GPLv3" or "" if unknown */
    QString license;

    Package(const QString& name, const QString& title);

    /**
     * @param terms search terms in lower case
     * @return true if the texts for this package match the specified terms
     */
    bool matches(const QStringList& terms) const;

    /**
     * Checks whether the specified value is a valid package name.
     *
     * @param a string that should be checked
     * @return true if name is a valid package name
     */
    static bool isValidName(QString& name);
};

#endif // PACKAGE_H
