#include "downloader.h"
#include "qobject.h"
#include "qdebug.h"
#include "qwaitcondition.h"
#include "qmutex.h"
#include "qapplication.h"

Downloader::Downloader()
{
    http = 0;
}

Downloader::Downloader(const Downloader &d)
{
    http = 0; // TODO: copy data?
}

Downloader::~Downloader()
{
    delete http;
}

bool Downloader::download(const QUrl& url, QTemporaryFile* file)
{
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
    connect(http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)),
            this, SLOT(readResponseHeader(const QHttpResponseHeader &)));
    connect(http, SIGNAL(authenticationRequired(const QString &, quint16, QAuthenticator *)),
            this, SLOT(slotAuthenticationRequired(const QString &, quint16, QAuthenticator *)));

    // TODO: timeout: http://stackoverflow.com/questions/250757/blocking-a-qt-application-during-downloading-a-short-file

    qDebug() << "Downloader::download.2";
    httpGetId = http->get(url.path(), file);

    qDebug() << "Downloader::download.3";

    while (!completed) {
        QApplication::instance()->processEvents(QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeUserInputEvents);
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
    this->successful = true;
}

 void Downloader::readResponseHeader(const QHttpResponseHeader &responseHeader)
 {
     qDebug() << "Downloader::readResponseHeader" << responseHeader.statusCode();
     if (responseHeader.statusCode() != 200) {
         // TODO: tr("Download failed: %1.").arg(responseHeader.reasonPhrase()));
         http->abort();
         return;
     }
 }

 void Downloader::updateDataReadProgress(int bytesRead, int totalBytes)
 {
     qDebug() << "Downloader::updateDataReadProgress";

     // TODO: progressDialog->setMaximum(totalBytes);
     // TODO: progressDialog->setValue(bytesRead);
 }

 void Downloader::slotAuthenticationRequired(const QString &hostName, quint16, QAuthenticator *authenticator)
 {
     qDebug() << "Downloader::slotAuthenticationRequired";
     // TODO: QDialog dlg;
     // TODO: Ui::Dialog ui;
     // TODO: ui.setupUi(&dlg);
     // TODO: dlg.adjustSize();
     // TODO: ui.siteDescription->setText(tr("%1 at %2").arg(authenticator->realm()).arg(hostName));
/*// TODO:
     if (dlg.exec() == QDialog::Accepted) {
         authenticator->setUser(ui.userEdit->text());
         authenticator->setPassword(ui.passwordEdit->text());
     }*/
 }
