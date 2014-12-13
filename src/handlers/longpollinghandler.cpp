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

LongPollingHandler::LongPollingHandler(GraphicsSceneWebServerTask *parent) :
    QObject(parent),
    task_(parent),
    display_(NULL)
{
}

bool LongPollingHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    return request->url().startsWith("/display?") || request->url().startsWith("/command?");
}

void LongPollingHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QString url(request->url());
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString id = QUrlQuery(request->url()).queryItemValue("id");
#else
    QString id = QUrl(request->url()).queryItemValue("id");
#endif

    display_ = task_->server()->getDisplay(id);

    if (display_)
    {
        if (url.startsWith("/display?")) // long polling
        {
            if (request->method() == "GET") // long polling output
            {
                display_->sendCommand("ready()");

                response_ = response;
                connect(response, SIGNAL(destroyed()), this, SLOT(handleResponseDestroyed()));

                connect(display_, SIGNAL(updateAvailable()), this, SLOT(handleDisplayUpdateAvailable()));

                if (!display_->isUpdateAvailable())
                    return;

                handleDisplayUpdateAvailable();
                return;
            }
            else if (request->method() == "POST") // long polling input
            {
                GraphicsSceneInputPostHandler *handler = new GraphicsSceneInputPostHandler(request, response, display_);
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
    else if (id.isEmpty()) // start new connection
    {
        display_ = new GraphicsSceneDisplay(task_->server());
        display_->moveToThread(QThread::currentThread());
        task_->server()->addDisplay(display_);

        QString info = "info(" + display_->id() + ")";

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
    if (response_ && display_ && display_->isUpdateAvailable())
    {
        QStringList commandList = display_->getUpdateCommandList();

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
    response_ = NULL;
}

