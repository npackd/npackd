#define _WIN32_IE 0x0500

#include <windows.h>
#include <shellapi.h>

#include <QtGui/QApplication>
#include "QMetaType"
#include "mainwindow.h"
#include "qnetworkproxy.h"

#include "repository.h"
#include "wpmutils.h"
#include "job.h"
#include "fileloader.h"

int main(int argc, char *argv[])
{
    /* test
    QString fn("C:\\Program Files (x86)\\wpm\\no.itefix.CWRsyncServer-4.0.4\\Bin\\PrepUploadDir.exe");
    QString p("C:\\Program Files (x86)\\WPM\\no.itefix.CWRsyncServer-4.0.4");
    qDebug() << WPMUtils::isUnder(fn, p);
    */

    LoadLibrary(L"exchndl.dll");

    QApplication a(argc, argv);
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    qRegisterMetaType<JobState>("JobState");
    qRegisterMetaType<FileLoaderItem>("FileLoaderItem");

    MainWindow w;
    if (a.arguments().count() == 2 && a.arguments().at(1) ==
        "check-for-updates") {

        Job job;
        Repository* r = Repository::getDefault();
        r->load(&job);
        int nupdates = r->countUpdates();
        if (job.getErrorMessage().isEmpty() && nupdates > 0) {
            a.setQuitOnLastWindowClosed(false);
            NOTIFYICONDATAW nid;
            memset(&nid, 0, sizeof(nid));
            nid.cbSize = sizeof(nid);
            nid.hWnd = w.winId();
            nid.uID = 0;
            nid.uFlags = NIF_MESSAGE + NIF_ICON + NIF_TIP + NIF_INFO;
            nid.uCallbackMessage = WM_ICONTRAY;
            nid.hIcon = LoadIconW(GetModuleHandle(0),
                            L"IDI_ICON1");
            qDebug() << "main().1 icon" << nid.hIcon;
            QString tip = QString("%1 update(s) found").arg(nupdates);
            QString txt = QString("Npackd found %1 update(s). "
                    "Click here to review and install.").arg(nupdates);

            wcsncpy(nid.szTip, (wchar_t*) tip.utf16(),
                    sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
            wcsncpy(nid.szInfo, (wchar_t*) txt.utf16(),
                    sizeof(nid.szInfo) / sizeof(nid.szInfo[0]) - 1);
            nid.uVersion = 3; // NOTIFYICON_VERSION
            wcsncpy(nid.szInfoTitle, (wchar_t*) tip.utf16(),
                    sizeof(nid.szInfoTitle) / sizeof(nid.szInfoTitle[0]) - 1);
            nid.dwInfoFlags = 1; // NIIF_INFO
            nid.uTimeout = 30000;

            if (!Shell_NotifyIconW(NIM_ADD, &nid))
                qDebug() << "Shell_NotifyIconW failed";
            int r = a.exec();
            if (!Shell_NotifyIconW(NIM_DELETE, &nid))
                qDebug() << "Shell_NotifyIconW failed";
            return r;
        }
        return 0;
    } else {
        w.prepare();
        w.showMaximized();
        return QApplication::exec();
    }
}
