#include <windows.h>
#include <wininet.h>

#include "qobject.h"
#include "qdebug.h"
#include "qwaitcondition.h"
#include "qmutex.h"
#include "qapplication.h"
#include "qnetworkproxy.h"

#include "downloader.h"
#include "job.h"

void formatMessage(DWORD err, QString* errMsg)
{
    HLOCAL pBuffer;
    DWORD n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                   FORMAT_MESSAGE_FROM_SYSTEM,
                   0, err, 0, (LPTSTR)&pBuffer, 0, 0);
    if (n == 0)
        errMsg->append(QString("Error %1").arg(err));
    else {
        errMsg->setUtf16((ushort*) pBuffer, n);
        LocalFree(pBuffer);
    }
}

bool downloadWin(Job* job, const QUrl& url, QTemporaryFile* file,
    QString* mime, QString* errMsg)
{
    qDebug() << "download.1";

    // const double second = 1.0 / 24 / 60 / 60;

    void* internet;
    DWORD bufferLength, index;
    char buffer[512 * 1024];
    HINTERNET hConnectHandle, hResourceHandle;
    unsigned int dwError, dwErrorCode;

    // TODO: https

    QString server = url.host();
    QString resource = url.path();

    // TODO: int r, r2;
    int contentLength;
    int64_t alreadyRead;
    double lastHintUpdated;
    // TODO: double d1, d2;

    lastHintUpdated = 0;

    if (job) {
        // TODO: job->setJob.AmountOfWork := 105;
        job->setHint("Connecting");
        // TODO: Job.Start;
    }

    internet = InternetOpenW(L"HttpLoader", INTERNET_OPEN_TYPE_PRECONFIG,
            0, 0, 0);

    qDebug() << "download.2";

    if (internet == 0) {
        return false;
        // TODO: raise EStreamError.Create(SysErrorMessage(GetLastError));
    }

    if (job)
        job->done(1);

    hConnectHandle = InternetConnectW(internet,
                                     (WCHAR*) server.utf16(),
                                     INTERNET_INVALID_PORT_NUMBER,
                                     0,
                                     0,
                                     INTERNET_SERVICE_HTTP,
                                     0, 0);
    qDebug() << "download.3";

    if (hConnectHandle == 0) {
        return false;
        // TODO: raise EStreamError.Create(SysErrorMessage(GetLastError));
    }

    qDebug() << "download.4";

    hResourceHandle = HttpOpenRequestW(hConnectHandle, L"GET",
                                      (WCHAR*) resource.utf16(),
                                      0, 0, 0,
                                      INTERNET_FLAG_KEEP_CONNECTION |
                                      INTERNET_FLAG_DONT_CACHE, 0);
    if (hResourceHandle == 0) {
        formatMessage(GetLastError(), errMsg);
        return false;
        // TODO: raise EStreamError.Create(SysErrorMessage(GetLastError));
    }

    qDebug() << "download.5";
    do {
        qDebug() << "download.5.1";

        if (!HttpSendRequestW(hResourceHandle, 0, 0, 0, 0)) {
            formatMessage(GetLastError(), errMsg);
            return false;
        }

        // dwErrorCode stores the error code associated with the call to
        // HttpSendRequest.

        if (hResourceHandle != 0)
            dwErrorCode = 0;
        else
            dwErrorCode = GetLastError();

        void* p;
        dwError = InternetErrorDlg(0, //TODO: window handle
                    hResourceHandle, dwErrorCode,
                                   FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
                                   FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS |
                                   FLAGS_ERROR_UI_FLAGS_GENERATE_DATA,
                                   &p);
    } while (dwError == ERROR_INTERNET_FORCE_RETRY);
    if (job)
        job->done(1);

    qDebug() << "download.6";

    WCHAR mimeBuffer[1024];
    bufferLength = sizeof(mimeBuffer);
    index = 0;
    if (!HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CONTENT_TYPE,
            &mimeBuffer, &bufferLength, &index)) {
        return false;
        // TODO: raise EStreamError.Create(SysErrorMessage(GetLastError));
    }
    mime->setUtf16((ushort*) mimeBuffer, bufferLength);
    qDebug() << "downloadWin.mime=" << *mime;
    if (job)
        job->done(1);

    WCHAR contentLengthBuffer[100];
    bufferLength = sizeof(contentLengthBuffer);
    index = 0;
    contentLength = -1;
    if (HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CONTENT_LENGTH,
            &contentLengthBuffer, &bufferLength, &index)) {
        QString s;
        s.setUtf16((ushort*) contentLengthBuffer, bufferLength);
        bool ok;
        contentLength = s.toInt(&ok, 10);
        if (!ok)
            contentLength = 0;
    }

    qDebug() << "download.7";

    alreadyRead = 0;
    do {
        if (!InternetReadFile(hResourceHandle, &buffer,
                sizeof(buffer), &bufferLength)) {
            return false;
            // TODO: raise EStreamError.Create(SysErrorMessage(GetLastError));
        }
        file->write(buffer, bufferLength);
        alreadyRead += bufferLength;
        if ((job != 0) && (contentLength > 0)) {
            // TODO: Job.Progress := Round(4 + (AlreadyRead / ContentLength) * 100);
            /* TODO: if (Now - LastHintUpdated > Second) {
                d1 := AlreadyRead;
                d2 := ContentLength;
                job->setHint(QString("%.0n von %.0n Bytes").arg(d1).arg(d2);
                LastHintUpdated := Now;
            }*/
        }
    } while (bufferLength != 0);

    // TODO: close everything in case of an error

    InternetCloseHandle(internet);

    /// TODO:    if (job)
        //job->done();

    qDebug() << "download.8";

    return true;
}

Downloader::Downloader()
{
    http = 0;
}

Downloader::~Downloader()
{
    delete http;
}

bool Downloader::download(const QUrl& url, QTemporaryFile* file, QString* errMsg)
{
    this->errMsg = errMsg;
    errMsg->clear();

    qDebug() << "Downloader::download.1" << url;
    this->file = file;

    QHttp::ConnectionMode mode = url.scheme().toLower() == "https" ?
            QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp;
    this->completed = false;
    this->successful = false;
    http = new QHttp(0);
    http->setHost(url.host(), mode, url.port() == -1 ? 0 : url.port());
    if (!url.userName().isEmpty()) {
        http->setUser(url.userName(), url.password());
    }
    connect(http, SIGNAL(requestFinished(int, bool)),
            this, SLOT(httpRequestFinished(int, bool)));
    connect(http, SIGNAL(dataReadProgress(int, int)),
            this, SLOT(updateDataReadProgress(int, int)));
    connect(http, SIGNAL(responseHeaderReceived(
            const QHttpResponseHeader &)),
            this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
    connect(http, SIGNAL(authenticationRequired(
            const QString &, quint16, QAuthenticator *)),
            this, SLOT(slotAuthenticationRequired(
            const QString &, quint16, QAuthenticator *)));

    QNetworkProxyQuery npq(url);
    QList<QNetworkProxy> listOfProxies =
            QNetworkProxyFactory::systemProxyForQuery(npq);

    http->setProxy(listOfProxies[0]);

    qDebug() << "Downloader::download.2";
    httpGetId = http->get(url.path(), file);

    qDebug() << "Downloader::download.3";

    while (!completed) {
        QApplication::instance()->processEvents(
                QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeUserInputEvents);
    }

    return successful;
}

void Downloader::cancelDownload()
{
    qDebug() << "Downloader::cancelDownload";
    http->abort();
}

void Downloader::httpRequestFinished(int requestId, bool error)
{
    qDebug() << "Downloader::httpRequestFinished" << error;
    if (requestId != httpGetId)
        return;

    this->completed = true;
    this->successful = !error;
    if (error)
        this->errMsg->append(this->http->errorString());
}

void Downloader::readResponseHeader(const QHttpResponseHeader &responseHeader)
{
    // for testing: *((char*) 0) = 0;
    qDebug() << "Downloader::readResponseHeader" << responseHeader.statusCode();
    if (responseHeader.statusCode() != 200) {
        this->errMsg->append("Error code: ").append(
                QString("%1").arg(responseHeader.statusCode())).append(
                "; ").append(responseHeader.reasonPhrase());
        http->abort();
        return;
    }
}

void Downloader::updateDataReadProgress(int bytesRead, int totalBytes)
{
}

void Downloader::slotAuthenticationRequired(const QString &hostName,
        quint16, QAuthenticator *authenticator)
{
    qDebug() << "Downloader::slotAuthenticationRequired";
    /* TODO: authentication
    QDialog dlg;
    Ui::Dialog ui;
    ui.setupUi(&dlg);
    dlg.adjustSize();
    ui.siteDescription->setText(tr("%1 at %2").arg(authenticator->realm()).arg(hostName));
    if (dlg.exec() == QDialog::Accepted) {
        authenticator->setUser(ui.userEdit->text());
        authenticator->setPassword(ui.passwordEdit->text());
    }*/
}

QTemporaryFile* Downloader::download(const QUrl &url, QString* errMsg)
{
    errMsg->clear();
    QTemporaryFile* file = new QTemporaryFile();
    if (file->open()) {
        //TODO: Qt Downloader d;
        //bool r = d.download(url, file, errMsg);

        QString mime;
        bool r = downloadWin(0, url, file, &mime, errMsg);
        file->close();

        if (!r) {
            delete file;
            file = 0;
        }
    } else {
        errMsg->append("Error opening file: ").append(file->fileName());
        delete file;
        file = 0;
    }
    return file;
}
