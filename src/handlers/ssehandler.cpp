#include "ssehandler.h"

#include <QUrl>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif

#include <QStringList>

#include "graphicsscenewebcontrol.h"
#include "graphicsscenedisplay.h"
#include "graphicssceneinputposthandler.h"
#include "commandposthandler.h"

#undef DEBUG
#include "KCL/debug.h"

SSEHandler::SSEHandler(GraphicsSceneWebServerConnection *parent) :
    QObject(parent),
    connection_(parent),
    display_(NULL)
{
    DGUARDMETHODTIMED;
}

SSEHandler::~SSEHandler()
{
    if (display_)
        connection_->server()->sessionManager()->releaseDisplay(display_);

    DGUARDMETHODTIMED;
}

bool SSEHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    return request->url().startsWith("/events");
}

void SSEHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    DGUARDMETHODTIMED;

    QString url(request->url());
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString id = QUrlQuery(request->url()).queryItemValue("id");
#else
    QString id = QUrl(request->url()).queryItemValue("id");
#endif

    GraphicsSceneDisplay *display = connection_->server()->sessionManager()->getDisplay(id);

    if (display)
    {
        display_ = connection_->server()->sessionManager()->acquireDisplay(id);
        setUpResponse(response);

        if (display_->isUpdateAvailable())
            handleDisplayUpdateAvailable();

        return;
    }
    else if (id.isEmpty()) // start new connection
    {
        display_ = connection_->server()->sessionManager()->getNewDisplay();

        DPRINTF("NEW DISPLAY: %s", display_->id().toLatin1().constData());

        QString info = "data: info(" + display_->id() + ")\n\n";

        setUpResponse(response);
        response_->write(info.toUtf8());
        response_->flush();

        return;
    }

    response->writeHead(404);
    response->end("Not found");
}

void SSEHandler::setUpResponse(Tufao::HttpServerResponse *response)
{
    response_ = response;

    response_->headers().insert("Content-Type", "text/event-stream");
    response_->headers().insert("Cache-Control", "no-cache");
    response_->writeHead(Tufao::HttpServerResponse::OK);

    connect(response_, SIGNAL(destroyed()), this, SLOT(handleResponseDestroyed()));

    connect(display_, SIGNAL(updateAvailable()), this, SLOT(handleDisplayUpdateAvailable()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));
}

void SSEHandler::handleDisplayUpdateAvailable()
{
    DGUARDMETHODTIMED;

    if (response_ && display_ && display_->isUpdateAvailable())
    {
        QStringList commandList = display_->getCommandsForPendingUpdates();

        QByteArray out;

        foreach (const QString &command, commandList)
            out += "data: " + command.toUtf8() + "\n";

        response_->write(out + "\n");
        response_->flush();
    }
}

void SSEHandler::handleResponseDestroyed()
{
    DGUARDMETHODTIMED;

    response_ = NULL;
}
