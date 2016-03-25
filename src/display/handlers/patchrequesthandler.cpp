#include "patchrequesthandler.h"

#include "graphicsscenedisplaymanager.h"
#include "graphicsscenedisplay.h"

#include "viriditywebserver.h"

//#define VIRIDITY_DEBUG_SIMULATE_RANDOM_ERROR

PatchRequestHandler::PatchRequestHandler(ViridityWebServer *server, QObject *parent) :
    ViridityBaseRequestHandler(server, parent)
{
}

PatchRequestHandler::~PatchRequestHandler()
{
}

bool PatchRequestHandler::doesHandleRequest(ViridityHttpServerRequest *request)
{
    QString id = ViriditySession::parseIdFromUrl(request->url());
    return request->url().contains("/p/") && request->url().contains("_") && server()->sessionManager()->getSession(id) != NULL;
}

void PatchRequestHandler::handleRequest(ViridityHttpServerRequest *request, ViridityHttpServerResponse *response)
{
#ifdef VIRIDITY_DEBUG_SIMULATE_RANDOM_ERROR
    if (QDateTime::currentMSecsSinceEpoch() % 101 == 0)
    {
        response->writeHead(404);
        response->end("Not found");
        return;
    }
#endif

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

        foreach (AbstractGraphicsSceneDisplayManager *sm, AbstractGraphicsSceneDisplayManager::activeDisplayManagers())
        {
            if (sm->session() == session)
            {
                GraphicsSceneDisplay *display = sm->acquireDisplay(displayId);
                if (display)
                {
                    sm->releaseDisplay(display);

                    GraphicsSceneFramePatch *patch = display->takePatch(patchId);

                    if (patch)
                    {
                        response->writeHead(ViridityHttpServerResponse::OK);
                        response->headers().insert("Content-Type", patch->mimeType.constData());
                        response->addNoCachingResponseHeaders();
                        response->end(patch->data);

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
