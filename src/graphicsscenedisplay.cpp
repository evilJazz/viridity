#include "graphicsscenedisplay.h"

//#undef DEBUG
#include "KCL/debug.h"

#include <QByteArray>
#include <QStringList>
#include <QBuffer>
#include <QFile>
#include <QThread>

#include <QMutexLocker>
#include <QUrl>

/* GraphicsSceneDisplay */

GraphicsSceneDisplay::GraphicsSceneDisplay(const QString &id, QGraphicsScene *scene, GraphicsSceneWebControlCommandInterpreter *commandInterpreter) :
    QObject(),
    scene_(scene),
    id_(id),
    commandInterpreter_(commandInterpreter),
    urlMode_(false),
    updateCheckInterval_(10),
    updateAvailable_(true),
    frame_(0),
    renderer_(NULL),
    clientReady_(true),
    patchesMutex_(QMutex::Recursive)
{
    DGUARDMETHODTIMED;

    renderer_ = new GraphicsSceneBufferRenderer(this);
    renderer_->setTargetGraphicsScene(scene_);

    connect(renderer_, SIGNAL(damagedRegionAvailable()), this, SLOT(sceneDamagedRegionsAvailable()));

    updateCheckTimer_ = new QTimer(this);
    connect(updateCheckTimer_, SIGNAL(timeout()), this, SLOT(updateCheckTimerTimeout()));
    updateCheckTimer_->setSingleShot(false);
    updateCheckTimer_->start(updateCheckInterval_);

    renderer_->setEnabled(true);
}

GraphicsSceneDisplay::~GraphicsSceneDisplay()
{
    DGUARDMETHODTIMED;   
    updateCheckTimer_->stop();

    clearPatches();

    // Explicitly delete renderer so we can use destroyed() signal from a different thread without
    // fearing to delete scene when children of display are still alive and accessing scene.
    // Remember: children will get deleted after destroyed() signal was fired...
    delete renderer_;
    renderer_ = NULL;
}

void GraphicsSceneDisplay::clearPatches()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&patchesMutex_);
    qDeleteAll(patches_.values());
    patches_.clear();
}

void GraphicsSceneDisplay::triggerUpdateCheckTimer()
{
    if (!updateCheckTimer_->isActive())
        updateCheckTimer_->start(updateCheckInterval_);
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
        QMetaObject::invokeMethod(
            this, "clientReady",
            this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection
        );
    else if (msg.startsWith("requestFullUpdate"))
        QMetaObject::invokeMethod(
            renderer_, "fullUpdate",
            renderer_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection
        );
    else
        QMetaObject::invokeMethod(
            commandInterpreter_, "dispatchCommand",
            commandInterpreter_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(const QString &, msg),
            Q_ARG(const QStringList &, params),
            Q_ARG(const QString &, this->id())
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
    triggerUpdateCheckTimer();
}

Patch *GraphicsSceneDisplay::createPatch(const QRect &rect, bool createBase64)
{
    Patch *patch = new Patch;

    patch->rect = rect;

    QImage image(rect.size(), QImage::Format_RGB888);

    QPainter p;
    p.begin(&image);
    p.drawImage(0, 0, patchBuffer_, rect.x(), rect.y());
    p.end();

    /*
    QBuffer pngBuffer;
    pngBuffer.open(QIODevice::ReadWrite);
    image.save(&pngBuffer, "PNG");

    QBuffer jpgBuffer;
    jpgBuffer.open(QIODevice::ReadWrite);
    image.save(&jpgBuffer, "JPEG", 50);

    if (pngBuffer.size() < jpgBuffer.size())
    {
        patch->data.setData(pngBuffer.data());
        patch->mimeType = "image/png";
    }
    else
    {
        patch->data.setData(jpgBuffer.data());
        patch->mimeType = "image/jpg";
    }
    //*/

    //*
    patch->data.open(QIODevice::ReadWrite);

    if (false && image.width() * image.height() > 9 * 9 * renderer_->tileSize() * renderer_->tileSize())
    {
        image.save(&patch->data, "JPEG", 50);
        patch->mimeType = "image/jpeg";
    }
    else
    {
        image.save(&patch->data, "PNG");
        patch->mimeType = "image/png";
    }
    //*/

    //image = image.convertToFormat(QImage::Format_Indexed8);
    //image.save(&patch->data, "PNG");
    //patch->mimeType = "image/png";

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

void GraphicsSceneDisplay::updateCheckTimerTimeout()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&patchesMutex_);

    DPRINTF("display: %p thread: %p id: %s UPDATE AVAILABLE! clientReady_: %s  patches_.count(): %d", this, this->thread(), id().toUtf8().constData(), clientReady_ ? "true" : "false", patches_.count());
    updateCheckTimer_->stop();

    if (clientReady_ && patches_.count() == 0)
    {
        l.unlock();
        updateAvailable_ = true;
        emit updateAvailable();
    }
}

QStringList GraphicsSceneDisplay::getMessagesForPendingUpdates()
{
    DGUARDMETHODTIMED;

    QStringList messageList;

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
                patch->id = QString::number(frame_) + "_" + QString::number(i);

                QString msg = QString().sprintf("drawImage(%d,%d,%d,%d,%d,%s):%s",
                    frame_,
                    rect.x(), rect.y(), rect.width(), rect.height(),
                    patch->mimeType.toLatin1().constData(),
                    QString("fb:" + id() + "/" + patch->id).toLatin1().constData()
                );

                QMutexLocker l(&patchesMutex_);
                patches_.insert(patch->id, patch);

                messageList += msg;
            }
            else
            {
                Patch *patch = createPatch(rect, true);

                QString format = patch->mimeType + ";base64";
                QString msg = QString().sprintf("drawImage(%d,%d,%d,%d,%d,%s):",
                    frame_,
                    rect.x(), rect.y(), rect.width(), rect.height(),
                    format.toLatin1().constData()
                );

                messageList += msg + patch->dataBase64;

                delete patch;
            }
        }
        else if (op.type == uotMove)
        {
            const QRect &rect = op.srcRect;

            QString msg = QString().sprintf("moveImage(%d,%d,%d,%d,%d,%d,%d):",
                frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                op.dstPoint.x(), op.dstPoint.y()
            );

            messageList += msg;
        }
        else if (op.type == uotFill)
        {
            const QRect &rect = op.srcRect;

            QString msg = QString().sprintf("fillRect(%d,%d,%d,%d,%d,%s):",
                frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                op.fillColor.name().toLatin1().constData()
            );

            messageList += msg;
        }
    }

    if (ops.count() > 0)
        messageList += QString().sprintf("end(%d):", frame_);

    if (additionalMessages_.count() > 0)
    {
        messageList += additionalMessages_;
        additionalMessages_.clear();
    }

    return messageList;
}

void GraphicsSceneDisplay::dispatchAdditionalMessages(const QStringList &messages)
{
    additionalMessages_ += messages;
    triggerUpdateCheckTimer();
}
