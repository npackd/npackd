#define _WIN32_IE 0x0500

#include <windows.h>
#include <shellapi.h>

#include <QtGui/QApplication>
#include "mainwindow.h"
#include "qnetworkproxy.h"

#include "repository.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    MainWindow w;
    if (a.arguments().count() == 2 && a.arguments().at(1) ==
        "check-for-updates") {

        Job job;
        Repository* r = Repository::getDefault();
        r->load(&job);
        int nupdates = r->countUpdates();
        if (job.getErrorMessage().isEmpty() && nupdates == 0) {
            a.setQuitOnLastWindowClosed(false);
            NOTIFYICONDATAW nid;
            memset(&nid, 0, sizeof(nid));
            nid.cbSize = sizeof(nid);
            nid.hWnd = w.winId();
            nid.uID = 0;
            nid.uFlags = NIF_MESSAGE + NIF_ICON + NIF_TIP + NIF_INFO;
            nid.uCallbackMessage = WM_ICONTRAY;
            nid.hIcon = LoadIconW(0,
                            IDI_APPLICATION);
            QString tip = QString("%1 updates found").arg(nupdates);
            wcsncpy(nid.szTip, L"Windows Package Manager",
                    sizeof(nid.szTip) / sizeof(nid.szTip[0]) - 1);
            /*nid.dwState = 0;
            nid.dwStateMask = 0;
            */
            wcsncpy(nid.szInfo, (wchar_t*) tip.utf16(),
                    sizeof(nid.szInfo) / sizeof(nid.szInfo[0]) - 1);
            nid.uVersion = 3; // NOTIFYICON_VERSION
            wcscpy(nid.szInfoTitle, L"Windows Package Manager");
            nid.dwInfoFlags = 1; // NIIF_INFO
            nid.uTimeout = 30000;

            if (!Shell_NotifyIconW(NIM_ADD, &nid))
                qDebug() << "Shell_NotifyIconW failed";
        }
    } else {
        w.prepare();
        w.showMaximized();
    }
    return QApplication::exec();
}
