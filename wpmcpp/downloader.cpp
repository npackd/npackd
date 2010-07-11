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
 * @param parentWindow window handle or 0 if not UI is required
 */
bool downloadWin(Job* job, const QUrl& url, QTemporaryFile* file,
        QString* mime, QString* contentDisposition, QString* errMsg,
        HWND parentWindow=0)
{
    qDebug() << "download.1";

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
        job->setAmountOfWork(105);
        job->setHint("Connecting");
    }

    internet = InternetOpenW(L"WPM", INTERNET_OPEN_TYPE_PRECONFIG,
            0, 0, 0);

    qDebug() << "download.2";

    if (internet == 0) {
        WPMUtils::formatMessage(GetLastError(), errMsg);
        return false;
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
        WPMUtils::formatMessage(GetLastError(), errMsg);
        return false;
    }

    qDebug() << "download.4";

    hResourceHandle = HttpOpenRequestW(hConnectHandle, L"GET",
                                      (WCHAR*) resource.utf16(),
                                      0, 0, 0,
                                      INTERNET_FLAG_KEEP_CONNECTION |
                                      INTERNET_FLAG_DONT_CACHE, 0);
    if (hResourceHandle == 0) {
        WPMUtils::formatMessage(GetLastError(), errMsg);
        return false;
    }

    qDebug() << "download.5";
    do {
        qDebug() << "download.5.1";

        if (!HttpSendRequestW(hResourceHandle, 0, 0, 0, 0)) {
            WPMUtils::formatMessage(GetLastError(), errMsg);
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
        job->done(1);

    qDebug() << "download.6";

    WCHAR mimeBuffer[1024];
    bufferLength = sizeof(mimeBuffer);
    index = 0;
    if (!HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CONTENT_TYPE,
            &mimeBuffer, &bufferLength, &index)) {
        WPMUtils::formatMessage(GetLastError(), errMsg);
        return false;
    }
    mime->setUtf16((ushort*) mimeBuffer, bufferLength / 2);
    qDebug() << "downloadWin.mime=" << *mime;
    if (job)
        job->done(1);

    WCHAR cdBuffer[1024];
    wcscpy(cdBuffer, L"Content-Disposition");
    bufferLength = sizeof(cdBuffer);
    index = 0;
    if (HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CUSTOM,
            &cdBuffer, &bufferLength, &index)) {
        contentDisposition->setUtf16((ushort*) cdBuffer, bufferLength / 2);
        qDebug() << "downloadWin.cd=" << *contentDisposition;
    } else {
        if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
            qDebug() << "downloadWin.content-disposition not found  ";
    }

    WCHAR contentLengthBuffer[100];
    bufferLength = sizeof(contentLengthBuffer);
    index = 0;
    contentLength = -1;
    if (HttpQueryInfoW(hResourceHandle, HTTP_QUERY_CONTENT_LENGTH,
            contentLengthBuffer, &bufferLength, &index)) {
        QString s;
        s.setUtf16((ushort*) contentLengthBuffer, bufferLength / 2);
        qDebug() << "download.6.2 " << s;
        bool ok;
        contentLength = s.toInt(&ok, 10);
        if (!ok)
            contentLength = 0;
        qDebug() << "download.6.2 " << contentLength;
    }

    qDebug() << "download.7";

    alreadyRead = 0;
    do {
        if (!InternetReadFile(hResourceHandle, &buffer,
                sizeof(buffer), &bufferLength)) {
            WPMUtils::formatMessage(GetLastError(), errMsg);
            return false;
        }
        file->write(buffer, bufferLength);
        alreadyRead += bufferLength;
        if ((job != 0) && (contentLength > 0)) {
            job->setProgress(lround(4 + alreadyRead * 100.0 / contentLength));
            job->setHint(QString("%0 of %1 Bytes").arg(alreadyRead).
                         arg(contentLength));
        }
    } while (bufferLength != 0);

    // TODO: close everything in case of an error

    InternetCloseHandle(internet);

    if (job)
        job->done(-1);

    qDebug() << "download.8";

    return true;
}

QTemporaryFile* Downloader::download(Job* job, const QUrl &url)
{
    QTemporaryFile* file = new QTemporaryFile();
    if (file->open()) {
        QString mime;
        QString errMsg;
        QString contentDisposition;
        HWND hwnd;
        QWidget* w = QApplication::activeWindow();
        if (w)
            hwnd = w->winId();
        else
            hwnd = 0;
        bool r = downloadWin(job, url, file, &mime, &contentDisposition,
                             &errMsg, hwnd);
        file->close();

        if (!r) {
            job->setErrorMessage(errMsg);
            delete file;
            file = 0;
        }
    } else {
        job->setErrorMessage(QString("Error opening file: %1").
                arg(file->fileName()));
        delete file;
        file = 0;
    }
    return file;
}
