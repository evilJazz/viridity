#include "patchrequesthandler.h"

#include "viriditywebserver.h"
#include "graphicsscenedisplaysessionmanager.h"
#include "graphicsscenedisplay.h"

PatchRequestHandler::PatchRequestHandler(ViridityConnection *parent) :
    QObject(parent),
    connection_(parent)
{
}

bool PatchRequestHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QString id = QString(request->url()).mid(1, 40);
    return connection_->server()->sessionManager()->getSession(id) != NULL;
}

void PatchRequestHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QString id = QString(request->url()).mid(1, 40);
    ViriditySession *session = connection_->server()->sessionManager()->acquireSession(id);

    if (session)
    {
        connection_->server()->sessionManager()->releaseSession(session);

        QString url = request->url();

        int indexOfSlash = url.lastIndexOf("/");
        QString patchId = "";

        if (indexOfSlash > -1)
            patchId = url.mid(indexOfSlash + 1);

        QString displayId = patchId.split('_')[0];

        foreach (GraphicsSceneDisplaySessionManager *sm, GraphicsSceneDisplaySessionManager::activeSessionManagers())
        {
            if (sm->session() == session)
            {
                GraphicsSceneDisplay *display = sm->acquireDisplay(displayId);
                if (display)
                {
                    sm->releaseDisplay(display);

                    Patch *patch = display->takePatch(patchId);

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
                    else
                        break;
                }
            }
        }
    }

    response->writeHead(404);
    response->end("Not found");
}
