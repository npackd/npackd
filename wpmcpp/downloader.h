#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <windows.h>

#include "qtemporaryfile.h"
#include "qurl.h"
#include "qmetatype.h"
#include "qobject.h"
#include "qwaitcondition.h"

#include "job.h"

/**
 * Blocks execution and downloads a file over http.
 */
class Downloader: QObject
{
    Q_OBJECT
public:

    /**
     * @param job job for this method
     * @param url this URL will be downloaded
     * @return temporary file or 0 if an error occured
     */
    static QTemporaryFile* download(Job* job, const QUrl& url);
};

#endif // DOWNLOADER_H
