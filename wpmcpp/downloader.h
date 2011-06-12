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

    /**
     * It would be nice to handle redirects explicitely so
     *    that the file name could be derived
     *    from the last URL:
     *    http://www.experts-exchange.com/Programming/System/Windows__Programming/MFC/Q_20096714.html
     * Manual authentication:
     *    http://msdn.microsoft.com/en-us/library/aa384220(v=vs.85).aspx
     * @param parentWindow window handle or 0 if not UI is required
     * @param sha1 if not null, SHA1 will be computed and stored here
     */
    static bool downloadWin(Job* job, const QUrl& url, QTemporaryFile* file,
            QString* mime, QString* contentDisposition,
            HWND parentWindow=0, QString* sha1=0);
public:
    /**
     * @param job job for this method
     * @param url this URL will be downloaded
     * @param sha1 if not null, SHA1 will be computed and stored here
     * @return temporary file or 0 if an error occured
     */
    static QTemporaryFile* download(Job* job, const QUrl& url,
            QString* sha1=0);
};

#endif // DOWNLOADER_H
