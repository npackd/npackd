#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "qhttp.h"
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
     * @param this URL will be downloaded
     * @param errMsg error message will be stored here
     * @return temporary file or 0 if an error occured
     */
    static QTemporaryFile* download(Job* job, const QUrl& url, QString* errMsg);
};

#endif // DOWNLOADER_H
