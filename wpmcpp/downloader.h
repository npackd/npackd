#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "qhttp.h"
#include "qtemporaryfile.h"
#include "qurl.h"
#include "qmetatype.h"
#include "qobject.h"
#include "qwaitcondition.h"

class Downloader: QObject
{
    Q_OBJECT

    QHttp* http;
    bool successful;
    int httpGetId;
    QTemporaryFile* file;
    bool completed;
public:
    Downloader();
    Downloader(const Downloader& d);
    ~Downloader();

    // TODO: document
    bool download(const QUrl& url, QTemporaryFile* f);
    void cancelDownload();
private slots:
    void httpRequestFinished(int requestId, bool error);
    void updateDataReadProgress(int bytesRead, int totalBytes);
    void readResponseHeader(const QHttpResponseHeader &responseHeader);
    void slotAuthenticationRequired(const QString &hostName, quint16, QAuthenticator *authenticator);
};

Q_DECLARE_METATYPE(Downloader);

#endif // DOWNLOADER_H
