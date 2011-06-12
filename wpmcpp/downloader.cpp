#include <math.h>

#include <windows.h>
#include <wininet.h>

#include "qobject.h"
#include "qdebug.h"
#include "qwaitcondition.h"
#include "qmutex.h"
#include "qcryptographichash.h"

#include "downloader.h"
#include "job.h"
#include "wpmutils.h"

HWND defaultPasswordWindow = 0;

bool Downloader::downloadWin(Job* job, const QUrl& url, QTemporaryFile* file,
        QString* mime, QString* contentDisposition,
        HWND parentWindow, QString* sha1)
{
    job->setHint("Connecting");

    if (sha1)
        sha1->clear();

    QString server = url.host();
    QString resource = url.path();
    QString encQuery = url.encodedQuery();
    if (!encQuery.isEmpty())
        resource.append('?').append(encQuery);

    QString agent("Npackd/");
    agent.append(WPMUtils::NPACKD_VERSION);

    void* internet = InternetOpenW((WCHAR*) agent.utf16(),
            INTERNET_OPEN_TYPE_PRECONFIG,
            0, 0, 0);

    if (internet == 0) {
        QString errMsg;
        WPMUtils::formatMessage(GetLastError(), &errMsg);
        job->setErrorMessage(errMsg);
        job->complete();
        return false;
    }

    job->setProgress(0.01);

    INTERNET_PORT port = url.port(url.scheme() == "https" ?
            INTERNET_DEFAULT_HTTPS_PORT: INTERNET_DEFAULT_HTTP_PORT);
    HINTERNET hConnectHandle = InternetConnectW(internet,
                                     (WCHAR*) server.utf16(),
                                     port,
                                     0,
                                     0,
                                     INTERNET_SERVICE_HTTP,
                                     0, 0);

    if (hConnectHandle == 0) {
        QString errMsg;
        WPMUtils::formatMessage(GetLastError(), &errMsg);
        job->setErrorMessage(errMsg);
        job->complete();
        return false;
    }

    if (job->isCancelled()) {
        InternetCloseHandle(internet);
        job->complete();
        return false;
    }

    // qDebug() << "download.4";

    HINTERNET hResourceHandle = HttpOpenRequestW(hConnectHandle, L"GET",
            (WCHAR*) resource.utf16(),
            0, 0, 0,
            (url.scheme() == "https" ? INTERNET_FLAG_SECURE : 0) |
            INTERNET_FLAG_KEEP_CONNECTION |
            INTERNET_FLAG_DONT_CACHE, 0);
    if (hResourceHandle == 0) {
        QString errMsg;
        WPMUtils::formatMessage(GetLastError(), &errMsg);
        job->setErrorMessage(errMsg);
        job->complete();
        return false;
    }

    // qDebug() << "download.5";
    unsigned int dwError, dwErrorCode;
    do {
        // qDebug() << "download.5.1";

        if (!HttpSendRequestW(hResourceHandle, 0, 0, 0, 0)) {
            QString errMsg;
            WPMUtils::formatMessage(GetLastError(), &errMsg);
            job->setErrorMessage(errMsg);
            job->complete();
            return false;
        }

        // dwErrorCode stores the error code associated with the call to
        // HttpSendRequest.

        if (hResourceHandle != 0)
            dwErrorCode = 0;
        else
            dwErrorCode = GetLastError();

        void* p;
        DWORD flags = FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
                      FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS |
                      FLAGS_ERROR_UI_FLAGS_GENERATE_DATA;
        if (parentWindow == 0)
            flags |= FLAGS_ERROR_UI_FLAGS_NO_UI;
        dwError = InternetErrorDlg(parentWindow,
                    hResourceHandle, dwErrorCode,
                                   flags,
                                   &p);
    } while (dwError == ERROR_INTERNET_FORCE_RETRY);
    job->setProgress(0.03);

    // MIME type
    WCHAR mimeBuffer[1024];
    DWORD bufferLength = sizeof(mimeBuffer);
    DWORD index = 0;
    if (!HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CONTENT_TYPE,
            &mimeBuffer, &bufferLength, &index)) {
        QString errMsg;
        WPMUtils::formatMessage(GetLastError(), &errMsg);
        job->setErrorMessage(errMsg);
        job->complete();
        return false;
    }
    mime->setUtf16((ushort*) mimeBuffer, bufferLength / 2);

    job->setProgress(0.04);

    // Content-Disposition
    WCHAR cdBuffer[1024];
    wcscpy(cdBuffer, L"Content-Disposition");
    bufferLength = sizeof(cdBuffer);
    index = 0;
    if (HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CUSTOM,
            &cdBuffer, &bufferLength, &index)) {
        contentDisposition->setUtf16((ushort*) cdBuffer, bufferLength / 2);
    }

    // content length
    WCHAR contentLengthBuffer[100];
    bufferLength = sizeof(contentLengthBuffer);
    index = 0;
    int contentLength = -1;
    int64_t alreadyRead;
    if (HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CONTENT_LENGTH,
            contentLengthBuffer, &bufferLength, &index)) {
        QString s;
        s.setUtf16((ushort*) contentLengthBuffer, bufferLength / 2);
        bool ok;
        contentLength = s.toInt(&ok, 10);
        if (!ok)
            contentLength = 0;
    }

    // download/compute SHA1 loop
    QCryptographicHash hash(QCryptographicHash::Sha1);
    alreadyRead = 0;
    char buffer[512 * 1024];
    do {
        if (!InternetReadFile(hResourceHandle, &buffer,
                sizeof(buffer), &bufferLength)) {
            QString errMsg;
            WPMUtils::formatMessage(GetLastError(), &errMsg);
            job->setErrorMessage(errMsg);
            job->complete();
            return false;
        }

        // update SHA1 if necessary
        if (sha1)
            hash.addData(buffer, bufferLength);

        file->write(buffer, bufferLength);
        alreadyRead += bufferLength;
        if (contentLength > 0) {
            job->setProgress(0.04 +
                    ((double) alreadyRead / contentLength) * 0.95);
            job->setHint(QString("%L0 of %L1 bytes").arg(alreadyRead).
                         arg(contentLength));
        }
        if (job->isCancelled()) {
            InternetCloseHandle(internet);
            job->complete();
            return false;
        }
    } while (bufferLength != 0);

    if (sha1 && !job->isCancelled() && job->getErrorMessage().isEmpty())
        *sha1 = hash.result().toHex().toLower();

    // close everything in case of an error
    InternetCloseHandle(internet);

    job->setProgress(1);

    job->complete();

    return true;
}

QTemporaryFile* Downloader::download(Job* job, const QUrl &url, QString* sha1)
{
    QTemporaryFile* file = new QTemporaryFile();
    if (file->open()) {
        QString mime;
        QString contentDisposition;
        bool r = downloadWin(job, url, file, &mime, &contentDisposition,
                defaultPasswordWindow, sha1);
        file->close();

        if (!r) {
            delete file;
            file = 0;
        }
    } else {
        job->setErrorMessage(QString("Error opening file: %1").
                arg(file->fileName()));
        delete file;
        file = 0;
        job->complete();
    }
    return file;
}
