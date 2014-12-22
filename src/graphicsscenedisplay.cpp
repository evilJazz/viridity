#include "graphicsscenedisplay.h"

//#undef DEBUG
#include "KCL/debug.h"

#include "private/commandbridge.h"

#include "Tufao/WebSocket"
#include "Tufao/HttpServerRequest"

#include "graphicsscenewebcontrol.h"
#include "handlers/graphicssceneinputposthandler.h"
#include "handlers/commandposthandler.h"

#include <QByteArray>
#include <QStringList>
#include <QBuffer>
#include <QFile>
#include <QThread>

#include <QUuid>
#include <QCryptographicHash>
#include <QMutexLocker>
#include <QUrl>

QString createUniqueID()
{
    QString uuid = QUuid::createUuid().toString();
    return QString(QCryptographicHash::hash(uuid.toUtf8(), QCryptographicHash::Sha1).toHex());
}

/* GraphicsSceneDisplay */

GraphicsSceneDisplay::GraphicsSceneDisplay(GraphicsSceneMultiThreadedWebServer *parent) :
    QObject(),
    server_(parent),
    urlMode_(true),
    updateCheckInterval_(1),
    updateAvailable_(true),
    frame_(0),
    renderer_(NULL),
    clientReady_(true),
    patchesMutex_(QMutex::Recursive)
{
    DGUARDMETHODTIMED;

    id_ = createUniqueID();

    renderer_ = new GraphicsSceneBufferRenderer(this);
    renderer_->setTargetGraphicsScene(server_->scene());

    connect(renderer_, SIGNAL(damagedRegionAvailable()), this, SLOT(sceneDamagedRegionsAvailable()));

    timer_ = new QTimer(this);
    connect(timer_, SIGNAL(timeout()), this, SLOT(sendUpdate()));
    timer_->setSingleShot(false);
    timer_->start(updateCheckInterval_);

    renderer_->setEnabled(true);

    // Finally create worker thread and move display + all children to this new thread's event loop.
    workerThread_ = new QThread(this);

    DPRINTF("New worker thread %p for display id %s", workerThread_, id_.toLatin1().constData());
    moveToThread(workerThread_);

    workerThread_->start();
}

GraphicsSceneDisplay::~GraphicsSceneDisplay()
{
    DGUARDMETHODTIMED;
    workerThread_->quit();
    workerThread_->wait();

    clearPatches();
    server_->removeDisplay(this);
}

void GraphicsSceneDisplay::clearPatches()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&patchesMutex_);
    qDeleteAll(patches_.values());
    patches_.clear();
}

Patch *GraphicsSceneDisplay::takePatch(const QString &patchId)
{
    QMutexLocker l(&patchesMutex_);

    if (patches_.contains(patchId))
    {
        DPRINTF("display: %p thread: %p id: %s -> Taking patch: %s", this, this->thread(), id().toUtf8().constData(), patchId.toUtf8().constData());
        return patches_.take(patchId);
    }
    else
    {
        DPRINTF("display: %p thread: %p id: %s -> No such patch: %s", this, this->thread(), id().toUtf8().constData(), patchId.toUtf8().constData());
        return NULL;
    }
}

bool GraphicsSceneDisplay::handleReceivedMessage(const QByteArray &data)
{
    DGUARDMETHODTIMED;
    QString rawMsg = data;

    int paramStartIndex = rawMsg.indexOf("(");
    int paramStopIndex = rawMsg.indexOf(")");

    QString command = rawMsg.mid(0, paramStartIndex);
    QString rawParams = rawMsg.mid(paramStartIndex + 1, paramStopIndex - paramStartIndex - 1);

    QStringList params = rawParams.split(",", QString::KeepEmptyParts);

    DPRINTF("display: %p thread: %p id: %s -> Received message: %s, command: %s, rawParams: %s", this, this->thread(), id().toUtf8().constData(), data.constData(), command.toLatin1().constData(), rawParams.toLatin1().constData());

    return handleReceivedMessage(command, params);
}

bool GraphicsSceneDisplay::handleReceivedMessage(const QString &msg, const QStringList &params)
{
    bool result = true;

    if (msg.startsWith("ready"))
        metaObject()->invokeMethod(
            this, "clientReady",
            server_->scene()->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection
        );
    else if (msg.startsWith("requestFullUpdate"))
        metaObject()->invokeMethod(
            renderer_, "fullUpdate",
            server_->scene()->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection
        );
    else
        metaObject()->invokeMethod(
            server_->commandInterpreter(), "sendCommand",
            server_->scene()->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(const QString &, msg),
            Q_ARG(const QStringList &, params)
        );

    return result;
}

void GraphicsSceneDisplay::clientReady()
{
    DGUARDMETHODTIMED;
    clientReady_ = true;

    if (patches_.count() != 0)
    {
        qWarning("CLIENT MISBEHAVING!! Client tells me it is ready, but we still have %d patches. Clearing patches. Expect trouble.", patches_.count());
        clearPatches();
    }

    if (renderer_->updatesAvailable())
        sceneDamagedRegionsAvailable();
}

void GraphicsSceneDisplay::sceneDamagedRegionsAvailable()
{
    DPRINTF("display: %p thread: %p id: %s -> Damaged regions in scene available", this, this->thread(), id().toUtf8().constData());

    if (!timer_->isActive())
        timer_->start(updateCheckInterval_);
}

Patch *GraphicsSceneDisplay::createPatch(const QRect &rect, bool createBase64)
{
    Patch *patch = new Patch;

    patch->id = createUniqueID();
    patch->rect = rect;

    QImage image(rect.size(), QImage::Format_RGB888);

    QPainter p(&image);
    p.drawImage(0, 0, patchBuffer_, rect.x(), rect.y());

    patch->data.open(QIODevice::ReadWrite);

    if (false && image.width() * image.height() > 9 * 9 * renderer_->tileSize() * renderer_->tileSize())
    {
        image.save(&patch->data, "JPEG", 90);
        patch->mimeType = "image/jpeg";
    }
    else
    {
        image.save(&patch->data, "PNG");
        patch->mimeType = "image/png";
    }

    //image.save(&patch->data, "BMP"); patch->mimeType = "image/bmp";

    patch->data.close();

    if (createBase64)
    {
        patch->dataBase64 = patch->data.data().toBase64();

        DPRINTF("rect: %d,%d,%d,%d, %s, image.size: %d kB (%d byte), compressed size: %d kB (%d byte), base64 size: %d kB (%d byte)",
                rect.x(), rect.y(), rect.width(), rect.height(), patch->mimeType.toLatin1().constData(),
                image.byteCount() / 1024, image.byteCount(),
                patch->data.size() / 1024, patch->data.size(),
                patch->dataBase64.size() / 1024, patch->dataBase64.size()
                );
    }
    else
    {
        DPRINTF("rect: %d,%d,%d,%d, %s, image.size: %d kB (%d byte), compressed size: %d kB (%d byte)",
                rect.x(), rect.y(), rect.width(), rect.height(), patch->mimeType.toLatin1().constData(),
                image.byteCount() / 1024, image.byteCount(),
                patch->data.size() / 1024, patch->data.size()
                );
    }

    return patch;
}

void GraphicsSceneDisplay::sendUpdate()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&patchesMutex_);

    DPRINTF("display: %p thread: %p id: %s UPDATE AVAILABLE! clientReady_: %s  patches_.count(): %d", this, this->thread(), id().toUtf8().constData(), clientReady_ ? "true" : "false", patches_.count());
    timer_->stop();

    if (clientReady_ && patches_.count() == 0)
    {
        l.unlock();
        updateAvailable_ = true;
        emit updateAvailable();
    }
}

QStringList GraphicsSceneDisplay::getCommandsForPendingUpdates()
{
    DGUARDMETHODTIMED;

    QStringList commandList;

    updateAvailable_ = false;
    QList<UpdateOperation> ops = renderer_->updateBufferExt();
    patchBuffer_ = renderer_->buffer();

    DPRINTF("Updates available: %d", ops.count());

    if (ops.count() > 0)
    {
        clientReady_ = false;
        ++frame_;
        DPRINTF("New Frame Number: %d", frame_);
    }

    for (int i = 0; i < ops.count(); ++i)
    {
        const UpdateOperation &op = ops.at(i);

        if (op.type == uotUpdate)
        {
            const QRect &rect = op.srcRect;

            if (urlMode_)
            {
                Patch *patch = createPatch(rect, false);

                QString framePatchId = QString::number(frame_) + "_" + QString::number(i);

                QString cmd = QString().sprintf("drawImage(%d,%d,%d,%d,%d,%s):%s",
                    frame_,
                    rect.x(), rect.y(), rect.width(), rect.height(),
                    patch->mimeType.toLatin1().constData(),
                    QString("fb:" + id() + "/" + framePatchId).toLatin1().constData()
                );

                QMutexLocker l(&patchesMutex_);
                patches_.insert(framePatchId, patch);

                commandList += cmd;
            }
            else
            {
                Patch *patch = createPatch(rect, true);

                QString format = patch->mimeType + ";base64";
                QString cmd = QString().sprintf("drawImage(%d,%d,%d,%d,%d,%s):",
                    frame_,
                    rect.x(), rect.y(), rect.width(), rect.height(),
                    format.toLatin1().constData()
                );

                commandList += cmd + patch->dataBase64;

                delete patch;
            }
        }
        else if (op.type == uotMove)
        {
            const QRect &rect = op.srcRect;

            QString cmd = QString().sprintf("moveImage(%d,%d,%d,%d,%d,%d,%d):",
                frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                op.dstPoint.x(), op.dstPoint.y()
            );

            commandList += cmd;
        }
        else if (op.type == uotFill)
        {
            const QRect &rect = op.srcRect;

            QString cmd = QString().sprintf("fillRect(%d,%d,%d,%d,%d,%s):",
                frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                op.fillColor.name().toLatin1().constData()
            );

            commandList += cmd;
        }
    }

    if (ops.count() > 0)
        commandList += QString().sprintf("end(%d):", frame_);

    return commandList;
}
