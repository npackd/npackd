#include "qobject.h"
#include "qdebug.h"
#include "qwaitcondition.h"
#include "qmutex.h"
#include "qapplication.h"
#include "qnetworkproxy.h"

#include "downloader.h"

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

    /* TODO is it necessary additionally to QNetworkProxyFactory::setUseSystemConfiguration ( bool enable )?
    QNetworkProxyQuery npq(url);
    QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
    */

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
    this->successful = true;
}

void Downloader::readResponseHeader(const QHttpResponseHeader &responseHeader)
{
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

void Downloader::slotAuthenticationRequired(const QString &hostName, quint16, QAuthenticator *authenticator)
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
