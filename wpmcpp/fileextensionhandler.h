#ifndef FILEEXTENSIONHANDLER_H
#define FILEEXTENSIONHANDLER_H

#include "qstring.h"

class FileExtensionHandler
{
public:
    /** file extension. e.g. ".zip" */
    QString extension;

    /**
     * program that will be started with a file name as the only parameter -
     * relative path
     */
    QString program;

    FileExtensionHandler(const QString extension, const QString program);
};

#endif // FILEEXTENSIONHANDLER_H
