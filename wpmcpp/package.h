#ifndef PACKAGE_H
#define PACKAGE_H

#include <QObject>
#include <QString>

/**
 * A package declaration.
 */
class Package : public QObject
{
    Q_OBJECT
public:
    explicit Package(QObject *parent = 0);

    /**
     * Checks whether the specified value is a valid package name.
     *
     * @param a string that should be checked
     * @return true if name is a valid package name
     */
    static bool isValidName(QString& name);

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
     * @return description that can be used for the full-text search in lower
     *     case
     */
    QString getFullText();
};

#endif // PACKAGE_H
