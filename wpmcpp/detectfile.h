#ifndef DETECTFILE_H
#define DETECTFILE_H

#include "qstring.h"

/**
 * Package detection using SHA1.
 */
class DetectFile
{
public:
    /** relative file path */
    QString path;

    /**
     * SHA1 for the file. This value must be lower case and without any
     * white space
     */
    QString sha1;

    DetectFile();
};

#endif // DETECTFILE_H
