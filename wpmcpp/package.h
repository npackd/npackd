#ifndef PACKAGE_H
#define PACKAGE_H

#include <QObject>
#include <QString>
#include <QDomElement>

/**
 * A package declaration.
 */
class Package : public QObject
{
    Q_OBJECT
public:
    explicit Package(QObject *parent = 0);

    /**
     * @param <package>
     * @param err error message will be stored here
     * @return created object or 0, if a error has occured
     */
    static Package* createPackage(QDomElement* e, QString* err);

    /**
     * @param xml XML created by serialize()
     * @param err error message will be stored here
     * @return created Package
     */
    static Package* deserialize(const QString& xml, QString* err);

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

    /**
     * @return XML representation of this package version
     */
    QString serialize() const;

    /**
     * Save the contents as XML.
     *
     * @param e <package>
     */
    void saveTo(QDomElement& e) const;
};

#endif // PACKAGE_H
