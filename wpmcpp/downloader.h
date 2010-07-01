#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "qhttp.h"
#include "qtemporaryfile.h"
#include "qurl.h"
#include "qmetatype.h"
#include "qobject.h"
#include "qwaitcondition.h"

/**
 * Blocks execution and downloads a file over http.
 */
class Downloader: QObject
{
    Q_OBJECT

    QHttp* http;
    bool successful;
    int httpGetId;
    QTemporaryFile* file;
    bool completed;
    QString* errMsg;
private slots:
    void httpRequestFinished(int requestId, bool error);
    void updateDataReadProgress(int bytesRead, int totalBytes);
    void readResponseHeader(const QHttpResponseHeader &responseHeader);
    void slotAuthenticationRequired(const QString &hostName, quint16, QAuthenticator *authenticator);
public:
    /**
     * Creates a new object. The object can be reused for downloading
     * multiple files on the same thread.
     */
    Downloader();

    /**
     * Destroys the downloader.
     */
    ~Downloader();

    /**
     * Downloads an URL.
     *
     * @param url this URL will be downloaded
     * @param f the data will be stored here
     * @param errMsg error message will be stored here
     * @return true if the URL was downloaded successfully
     */
    bool download(const QUrl& url, QTemporaryFile* f, QString* errMsg);

    /**
     * Cancels the running download.
     */
    void cancelDownload();

    /**
     * @param this URL will be downloaded
     * @param errMsg error message will be stored here
     * @return temporary file or 0 if an error occured
     */
    static QTemporaryFile* download(const QUrl& url, QString* errMsg);
};

#endif // DOWNLOADER_H
