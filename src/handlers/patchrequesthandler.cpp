#include "patchrequesthandler.h"

#include "graphicsscenedisplaysessionmanager.h"
#include "graphicsscenedisplay.h"

#include "viriditywebserver.h"

PatchRequestHandler::PatchRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

PatchRequestHandler::~PatchRequestHandler()
{
}

bool PatchRequestHandler::doesHandleRequest(Tufao::HttpServerRequest *request)
{
    QString id = ViriditySession::parseIdFromUrl(request->url());
    return server()->sessionManager()->getSession(id) != NULL;
}

void PatchRequestHandler::handleRequest(Tufao::HttpServerRequest *request, Tufao::HttpServerResponse *response)
{
    QString id = ViriditySession::parseIdFromUrl(request->url());
    ViriditySession *session = server()->sessionManager()->acquireSession(id);

    if (session)
    {
        server()->sessionManager()->releaseSession(session);

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
                        response->headers().insert("Content-Type", patch->mimeType.constData());
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
