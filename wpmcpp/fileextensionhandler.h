#ifndef FILEEXTENSIONHANDLER_H
#define FILEEXTENSIONHANDLER_H

#include "qstring.h"
#include "qstringlist.h"

class FileExtensionHandler
{
public:
    /** file extension. e.g. ".zip" */
    QStringList extensions;

    /** title */
    QString title;

    /**
     * program that will be started with a file name as the only parameter -
     * relative path
     */
    QString program;

    /**
     * @param program program path relative to the installation directory
     */
    FileExtensionHandler(const QString program);
};

#endif // FILEEXTENSIONHANDLER_H
