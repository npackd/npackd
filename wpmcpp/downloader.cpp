#include <math.h>

#include <windows.h>
#include <wininet.h>

#include "qobject.h"
#include "qdebug.h"
#include "qwaitcondition.h"
#include "qmutex.h"
#include "qapplication.h"
#include "qnetworkproxy.h"
#include "qwidget.h"

#include "downloader.h"
#include "job.h"
#include "wpmutils.h"

/**
 * TODO: handle redirects explicitely so that the file name could be derived
 *    from the last URL:
 *    http://www.experts-exchange.com/Programming/System/Windows__Programming/MFC/Q_20096714.html
 * @param parentWindow window handle or 0 if not UI is required
 */
bool downloadWin(Job* job, const QUrl& url, QTemporaryFile* file,
        QString* mime, QString* contentDisposition,
        HWND parentWindow=0)
{
    if (job)
        job->setCancellable(true);

    // qDebug() << "download.1";

    void* internet;
    DWORD bufferLength, index;
    char buffer[512 * 1024];
    HINTERNET hConnectHandle, hResourceHandle;
    unsigned int dwError, dwErrorCode;

    QString server = url.host();
    QString resource = url.path();

    int contentLength;
    int64_t alreadyRead;

    if (job) {
        job->setHint("Connecting");
    }

    internet = InternetOpenW(L"WPM", INTERNET_OPEN_TYPE_PRECONFIG,
            0, 0, 0);

    // qDebug() << "download.2";

    if (internet == 0) {
        QString errMsg;
        WPMUtils::formatMessage(GetLastError(), &errMsg);
        job->setErrorMessage(errMsg);
        job->complete();
        return false;
    }

    if (job)
        job->setProgress(0.01);

    hConnectHandle = InternetConnectW(internet,
                                     (WCHAR*) server.utf16(),
                                     INTERNET_INVALID_PORT_NUMBER,
                                     0,
                                     0,
                                     INTERNET_SERVICE_HTTP,
                                     0, 0);
    // qDebug() << "download.3";

    if (hConnectHandle == 0) {
        QString errMsg;
        WPMUtils::formatMessage(GetLastError(), &errMsg);
        job->setErrorMessage(errMsg);
        job->complete();
        return false;
    }

    if (job && job->isCancelled()) {
        InternetCloseHandle(internet);
        job->complete();
        return false;
    }

    // qDebug() << "download.4";

    hResourceHandle = HttpOpenRequestW(hConnectHandle, L"GET",
                                      (WCHAR*) resource.utf16(),
                                      0, 0, 0,
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
    if (job)
        job->setProgress(0.03);

    // qDebug() << "download.6";

    WCHAR mimeBuffer[1024];
    bufferLength = sizeof(mimeBuffer);
    index = 0;
    if (!HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CONTENT_TYPE,
            &mimeBuffer, &bufferLength, &index)) {
        QString errMsg;
        WPMUtils::formatMessage(GetLastError(), &errMsg);
        job->setErrorMessage(errMsg);
        job->complete();
        return false;
    }
    mime->setUtf16((ushort*) mimeBuffer, bufferLength / 2);
    // qDebug() << "downloadWin.mime=" << *mime;
    if (job)
        job->setProgress(0.04);

    WCHAR cdBuffer[1024];
    wcscpy(cdBuffer, L"Content-Disposition");
    bufferLength = sizeof(cdBuffer);
    index = 0;
    if (HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CUSTOM,
            &cdBuffer, &bufferLength, &index)) {
        contentDisposition->setUtf16((ushort*) cdBuffer, bufferLength / 2);
        // qDebug() << "downloadWin.cd=" << *contentDisposition;
    }

    WCHAR contentLengthBuffer[100];
    bufferLength = sizeof(contentLengthBuffer);
    index = 0;
    contentLength = -1;
    if (HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CONTENT_LENGTH,
            contentLengthBuffer, &bufferLength, &index)) {
        QString s;
        s.setUtf16((ushort*) contentLengthBuffer, bufferLength / 2);
        // qDebug() << "download.6.2 " << s;
        bool ok;
        contentLength = s.toInt(&ok, 10);
        if (!ok)
            contentLength = 0;
        // qDebug() << "download.6.2 " << contentLength;
    }

    // qDebug() << "download.7";

    alreadyRead = 0;
    do {
        if (!InternetReadFile(hResourceHandle, &buffer,
                sizeof(buffer), &bufferLength)) {
            QString errMsg;
            WPMUtils::formatMessage(GetLastError(), &errMsg);
            job->setErrorMessage(errMsg);
            job->complete();
            return false;
        }
        file->write(buffer, bufferLength);
        alreadyRead += bufferLength;
        if ((job != 0) && (contentLength > 0)) {
            job->setProgress(0.04 +
                    ((double) alreadyRead / contentLength) * 0.95);
            job->setHint(QString("%L0 of %L1 bytes").arg(alreadyRead).
                         arg(contentLength));
        }
        if (job && job->isCancelled()) {
            InternetCloseHandle(internet);
            job->complete();
            return false;
        }
    } while (bufferLength != 0);

    // TODO: close everything in case of an error

    InternetCloseHandle(internet);

    if (job)
        job->setProgress(1);

    // qDebug() << "download.8";

    job->complete();

    return true;
}

QTemporaryFile* Downloader::download(Job* job, const QUrl &url)
{
    QTemporaryFile* file = new QTemporaryFile();
    if (file->open()) {
        QString mime;
        QString contentDisposition;
        HWND hwnd;
        QWidget* w = QApplication::activeWindow();
        if (w)
            hwnd = w->winId();
        else
            hwnd = 0;
        bool r = downloadWin(job, url, file, &mime, &contentDisposition,
                             hwnd);
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
