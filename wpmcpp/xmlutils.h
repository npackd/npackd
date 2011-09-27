#ifndef XMLUTILS_H
#define XMLUTILS_H

#include <QString>
#include <QDomElement>

class XMLUtils
{
public:
    XMLUtils();

    /**
     * Returns the content of a sub-tag. If there are more than one tag with
     * the specified name, the content of the first tag will be returned. The
     * whitespace at the beginning and at the end will be removed.
     *
     * @param parent parent node
     * @param name name of the child node
     * @return content of the child node or QString() if the child node does not
     *     exist
     */
    static QString getTagContent(const QDomElement& parent, const QString& name);
};

#endif // XMLUTILS_H
