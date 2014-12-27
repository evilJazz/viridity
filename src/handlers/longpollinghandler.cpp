#include "longpollinghandler.h"

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

LongPollingHandler::LongPollingHandler(GraphicsSceneWebServerConnection *parent) :
    QObject(parent),
    connection_(parent),
    display_(NULL)
{
    DGUARDMETHODTIMED;
}

LongPollingHandler::~LongPollingHandler()
{
    if (display_)
        connection_->server()->sessionManager()->releaseDisplay(display_);

    DGUARDMETHODTIMED;
}

bool LongPollingHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    return request->url().startsWith("/display?") || request->url().startsWith("/command?");
}

void LongPollingHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
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
        if (url.startsWith("/display?")) // long polling
        {
            if (request->method() == "GET") // long polling output
            {
                display_ = connection_->server()->sessionManager()->acquireDisplay(id);

                response_ = response;
                connect(response, SIGNAL(destroyed()), this, SLOT(handleResponseDestroyed()));

                connect(display_, SIGNAL(updateAvailable()), this, SLOT(handleDisplayUpdateAvailable()), (Qt::ConnectionType)(Qt::AutoConnection | Qt::UniqueConnection));

                if (display_->isUpdateAvailable())
                    handleDisplayUpdateAvailable();

                return;
            }
            else if (request->method() == "POST") // long polling input
            {
                GraphicsSceneInputPostHandler *handler = new GraphicsSceneInputPostHandler(request, response, display);
                connect(response, SIGNAL(destroyed()), handler, SLOT(deleteLater()));
                return;
            }
        }
        else if (url.startsWith("/command?") && request->method() == "POST") // long polling command
        {
            CommandPostHandler *handler = new CommandPostHandler(request, response);
            connect(response, SIGNAL(destroyed()), handler, SLOT(deleteLater()));
        }

        return;
    }
    else if (id.isEmpty()/* && request->method() == "GET" && url.startsWith("/display?")*/) // start new connection
    {
        display = connection_->server()->sessionManager()->getNewDisplay();

        DPRINTF("NEW DISPLAY: %s", display->id().toLatin1().constData());

        QString info = "info(" + display->id() + ")";

        connection_->server()->sessionManager()->releaseDisplay(display);

        response->writeHead(Tufao::HttpServerResponse::OK);
        response->headers().insert("Content-Type", "text/plain; charset=utf8");
        response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response->headers().insert("Pragma", "no-cache");
        response->end(info.toUtf8());

        return;
    }

    response->writeHead(404);
    response->end("Not found");
}

void LongPollingHandler::handleDisplayUpdateAvailable()
{
    DGUARDMETHODTIMED;

    if (response_ && display_ && display_->isUpdateAvailable())
    {
        QStringList commandList = display_->getCommandsForPendingUpdates();

        connection_->server()->sessionManager()->releaseDisplay(display_);
        display_ = NULL;

        QByteArray out;
        out = commandList.join("\n").toUtf8();

        response_->writeHead(Tufao::HttpServerResponse::OK);
        response_->headers().insert("Content-Type", "text/plain; charset=utf8");
        response_->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
        response_->headers().insert("Pragma", "no-cache");
        response_->end(out);
    }
}

void LongPollingHandler::handleResponseDestroyed()
{
    DGUARDMETHODTIMED;

    response_ = NULL;
}

