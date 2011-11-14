#include "xmlutils.h"

#include <QDomNode>
#include <QDomNodeList>

XMLUtils::XMLUtils()
{
}

QString XMLUtils::getTagContent(const QDomElement& parent, const QString& name)
{
    QDomNodeList nl = parent.elementsByTagName(name);
    if (nl.count() >= 1) {
        QDomNode child = nl.at(0);
        QDomNodeList cnl = child.childNodes();
        if (cnl.count() == 1 && cnl.at(0).nodeType() == QDomNode::TextNode) {
            return cnl.at(0).nodeValue().trimmed();
        } else if (cnl.count() == 0) {
            return "";
        } else {
            return QString();
        }
    } else {
        return QString();
    }
}

