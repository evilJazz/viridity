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

#define USE_MULTITHREADING

#ifdef USE_MULTITHREADING
#include <QtConcurrentFilter>
#endif

#ifdef USE_IMPROVED_JPEG
#include "private/jpegwriter.h"
#endif

/* GraphicsSceneDisplayLocker */

GraphicsSceneDisplayLocker::GraphicsSceneDisplayLocker(GraphicsSceneDisplay *display) :
    m_(&display->patchesMutex_)
{

}

/* GraphicsSceneDisplay */

GraphicsSceneDisplay::GraphicsSceneDisplay(const QString &id, QGraphicsScene *scene, GraphicsSceneWebControlCommandInterpreter *commandInterpreter) :
    QObject(),
    scene_(scene),
    commandInterpreter_(commandInterpreter),
    id_(id),
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

bool GraphicsSceneDisplay::isUpdateAvailable() const
{
    QMutexLocker l(&patchesMutex_);
    return clientReady_ && patches_.count() == 0 && updateAvailable_;
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

GraphicsSceneFramePatch *GraphicsSceneDisplay::takePatch(const QString &patchId)
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

void GraphicsSceneDisplay::clientReady()
{
    DGUARDMETHODTIMED;
    QMutexLocker l(&patchesMutex_);
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

GraphicsSceneFramePatch *GraphicsSceneDisplay::createPatch(const QRect &rect)
{
    GraphicsSceneFramePatch *patch = new GraphicsSceneFramePatch;

    patch->rect = rect;

    // Account for encoding artifacts...
    patch->artefactMargin = 8; // block size of JPEG

    QRect rectEnlarged = rect.adjusted(
        -patch->artefactMargin,
        -patch->artefactMargin,
        patch->artefactMargin,
        patch->artefactMargin
    );

    //QImage image(rect.size(), QImage::Format_RGB888);
    QImage image(rectEnlarged.size(), QImage::Format_ARGB32);
    image.fill(0);

    QPainter p;
    p.begin(&image);
    p.drawImage(0, 0, renderer_->buffer(), rectEnlarged.x(), rectEnlarged.y());
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

    /*
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

    //    image.save(&patch->data, "JPEG", 50);
//        patch->mimeType = "image/jpeg";

    /*
    image = image.convertToFormat(QImage::Format_Indexed8);
    image.save(&patch->data, "PNG");
    patch->mimeType = "image/png";
    //*/

    image = createPackedAlphaPatch(image);

#ifdef USE_IMPROVED_JPEG
    patch->data.open(QIODevice::ReadWrite);
    writeJPEG(image, &patch->data, 90, true, false);
#else
    image.save(&patch->data, "JPEG", 90);
#endif
    patch->mimeType = "image/jpeg";

    patch->packedAlpha = true;

    patch->data.close();

    DPRINTF("rect: %d,%d,%d,%d, %s, image.size: %d kB (%d byte), compressed size: %lld kB (%lld byte)",
            rect.x(), rect.y(), rect.width(), rect.height(), patch->mimeType.constData(),
            image.byteCount() / 1024, image.byteCount(),
            patch->data.size() / 1024, patch->data.size()
            );

    return patch;
}

QImage GraphicsSceneDisplay::createPackedAlphaPatch(const QImage &input)
{
    QImage result(input.width(), input.height() * 2, QImage::Format_RGB888);
    result.fill(0);

    QImage inputWithoutAlpha = input.convertToFormat(QImage::Format_RGB888);
    QImage alpha = input.alphaChannel();

    QPainter p;
    p.begin(&result);
    p.drawImage(0, 0, inputWithoutAlpha);
    p.drawImage(0, input.height(), alpha);
    p.end();

    return result;
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

#ifdef USE_MULTITHREADING
struct GraphicsSceneDisplayThreadedCreatePatch
{
    GraphicsSceneDisplayThreadedCreatePatch(GraphicsSceneDisplay *display) :
        display(display)
    {
    }

    bool operator()(UpdateOperation *op)
    {
        op->data = display->createPatch(op->srcRect);
        return true;
    }

    GraphicsSceneDisplay *display;
};
#endif

QList<QByteArray> GraphicsSceneDisplay::takePendingMessages()
{
    DGUARDMETHODTIMED;

    QList<QByteArray> messageList;

    if (!isUpdateAvailable())
        return messageList;

    updateAvailable_ = false;

    GraphicsSceneBufferRendererLocker l(renderer_); // Lock and hold until all patches are created!
    QList<UpdateOperation> ops = renderer_->updateBuffer();

    DPRINTF("Updates available: %d", ops.count());

    if (ops.count() > 0)
    {
        clientReady_ = false;
        ++frame_;
        DPRINTF("New Frame Number: %d", frame_);
    }

#ifdef USE_MULTITHREADING
    QList<UpdateOperation *> updateOps;

    for (int i = 0; i < ops.count(); ++i)
    {
        UpdateOperation *op = &ops[i];
        if (op->type == uotUpdate)
            updateOps.append(op);
    }

    if (updateOps.count() > 0)
        QtConcurrent::blockingFilter(updateOps, GraphicsSceneDisplayThreadedCreatePatch(this));
#endif

    for (int i = 0; i < ops.count(); ++i)
    {
        const UpdateOperation &op = ops.at(i);

        if (op.type == uotUpdate)
        {
            const QRect &rect = op.srcRect;

            GraphicsSceneFramePatch *patch;
            if (op.data)
                patch = static_cast<GraphicsSceneFramePatch *>(op.data);
            else
                patch = createPatch(rect);

            QByteArray mimeType = patch->mimeType + (patch->packedAlpha ? ";pa" : "");

            bool embedData = !urlMode_;
            //if (patch->data.size() < 10 * 1024)
            //    embedData = true;

            if (embedData)
                mimeType = mimeType + ";base64";

            QString msg = QString().sprintf("%s>drawImage(%d,%d,%d,%d,%d,%d,%s):",
                id_.toLatin1().constData(), frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                patch->artefactMargin,
                mimeType.constData()
            );

            if (!embedData)
            {
                QMutexLocker l(&patchesMutex_);
                patch->id = id_ + "_" + QString::number(frame_) + "_" + QString::number(i);

                msg += QString("fb:" + patch->id).toLatin1().constData();

                patches_.insert(patch->id, patch);
            }
            else
            {
                msg += patch->toBase64();
                delete patch;
            }

            messageList += msg.toUtf8();
        }
        else if (op.type == uotMove)
        {
            const QRect &rect = op.srcRect;

            QString msg = QString().sprintf("%s>moveImage(%d,%d,%d,%d,%d,%d,%d):",
                id_.toLatin1().constData(), frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                op.dstPoint.x(), op.dstPoint.y()
            );

            messageList += msg.toUtf8();
        }
        else if (op.type == uotFill)
        {
            const QRect &rect = op.srcRect;

            QString msg = QString().sprintf("%s>fillRect(%d,%d,%d,%d,%d,%d,%d,%d,%d):",
                id_.toLatin1().constData(), frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                op.fillColor.red(), op.fillColor.green(), op.fillColor.blue(), op.fillColor.alpha()
            );

            messageList += msg.toUtf8();
        }
    }

    if (ops.count() > 0)
        messageList += QString().sprintf("%s>end(%d):", id_.toLatin1().constData(), frame_).toUtf8();

    emit newFrameMessagesGenerated(messageList);

    return messageList;
}

void GraphicsSceneDisplay::requestFullUpdate()
{
    QMutexLocker l(&patchesMutex_);
    clearPatches();

    QMetaObject::invokeMethod(
        renderer_, "fullUpdate",
        renderer_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection
    );

    clientReady();
}

bool GraphicsSceneDisplay::canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    return targetId == id_ && (
        message.startsWith("ready") ||
        message.startsWith("requestFullUpdate") ||
        message.startsWith("resize") ||
        message.startsWith("keepAlive") ||
        static_cast<ViridityMessageHandler *>(commandInterpreter_)->canHandleMessage(message, sessionId, targetId)
    );
}

bool GraphicsSceneDisplay::handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    DGUARDMETHODTIMED;
    bool result = true;

    if (message.startsWith("ready"))
        QMetaObject::invokeMethod(
            this, "clientReady",
            this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection
        );
    else if (message.startsWith("requestFullUpdate"))
        requestFullUpdate();
    else if (message.startsWith("resize"))
    {
        QString command;
        QStringList params;

        ViridityMessageHandler::splitMessage(message, command, params);

        if (params.count() == 2)
        {
            int width = params[0].toInt();
            int height = params[1].toInt();

            QMetaObject::invokeMethod(
                renderer_, "setSize",
                renderer_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                Q_ARG(int, width),
                Q_ARG(int, height)
            );
        }
    }
    else
        QMetaObject::invokeMethod(
            commandInterpreter_, "handleMessage",
            commandInterpreter_->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(const QByteArray &, message),
            Q_ARG(const QString &, sessionId),
            Q_ARG(const QString &, targetId)
        );

    return result;
}

