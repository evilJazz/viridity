#include "patchrequesthandler.h"

#include "graphicsscenewebcontrol.h"
#include "graphicsscenedisplay.h"

PatchRequestHandler::PatchRequestHandler(GraphicsSceneWebServerConnection *parent) :
    QObject(parent),
    task_(parent)
{
}

bool PatchRequestHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QString id = QString(request->url()).mid(1, 40);
    return task_->server()->getDisplay(id) != NULL;
}

void PatchRequestHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QString id = QString(request->url()).mid(1, 40);
    GraphicsSceneDisplay *display = task_->server()->acquireDisplay(id);

    if (display)
    {
        QString url = request->url();

        int indexOfSlash = url.lastIndexOf("/");
        QString patchId = "";

        if (indexOfSlash > -1)
            patchId = url.mid(indexOfSlash + 1);

        Patch *patch = display->takePatch(patchId);

        task_->server()->releaseDisplay(display);

        if (patch)
        {
            patch->data.open(QIODevice::ReadOnly);

            response->writeHead(Tufao::HttpServerResponse::OK);
            response->headers().insert("Content-Type", patch->mimeType.toUtf8().constData());
            response->headers().insert("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
            response->headers().insert("Pragma", "no-cache");
            response->end(patch->data.readAll());

            delete patch;
            return;
        }
    }

    response->writeHead(404);
    response->end("Not found");
}
