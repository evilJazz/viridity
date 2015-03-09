#include "graphicsscenedisplay.h"

#include "comparer/imageaux.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"
#include "KCL/imageutils.h"

#include <QByteArray>
#include <QStringList>
#include <QBuffer>
#include <QFile>
#include <QThread>

#include <QMutexLocker>
#include <QUrl>

#include <QtConcurrentFilter>

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

void GraphicsSceneDisplay::setEncoderSettings(const EncoderSettings &encoderSettings)
{
    QMutexLocker l(&patchesMutex_);
    encoderSettings_ = encoderSettings;
}

const ComparerSettings GraphicsSceneDisplay::comparerSettings() const
{
    return renderer_->settings();
}

void GraphicsSceneDisplay::setComparerSettings(const ComparerSettings &comparerSettings)
{
    renderer_->setSettings(comparerSettings);
}

bool GraphicsSceneDisplay::isUpdateAvailable() const
{
    if (scene_)
    {
        QMutexLocker l(&patchesMutex_);
        return clientReady_ && patches_.count() == 0 && updateAvailable_;
    }
    else
        return true;
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

    bool lossyEncodingPossible = encoderSettings_.patchEncodingFormat & EncoderSettings::EncodingFormat_JPEG;

    patch->rect = rect;

    // Account for encoding artifacts...
    patch->artefactMargin = lossyEncodingPossible ? 8 : 0; // block size of JPEG

    QRect rectEnlarged = rect.adjusted(
        -patch->artefactMargin,
        -patch->artefactMargin,
        patch->artefactMargin,
        patch->artefactMargin
    );

    bool hasAlphaValues = encoderSettings_.alphaChannelEnabled && ImageUtils::hasAlphaValues(renderer_->buffer(), rectEnlarged);
    QImage image;

    if (hasAlphaValues)
    {
        image = QImage(rectEnlarged.size(), QImage::Format_ARGB32);
        image.fill(0);
    }
    else
    {
        image = QImage(rectEnlarged.size(), QImage::Format_RGB888);
    }

    QPainter p;
    p.begin(&image);
    p.drawImage(0, 0, renderer_->buffer(), rectEnlarged.x(), rectEnlarged.y());
    p.end();

    int estimatedPNGSize;
    if (encoderSettings_.patchEncodingFormat & EncoderSettings::EncodingFormat_PNG &&
        (encoderSettings_.patchEncodingFormat == EncoderSettings::EncodingFormat_PNG || ImageAux::estimatePNGCompression(image, &estimatedPNGSize) < 0.4 || estimatedPNGSize < 1024))
    {
        if (patch->artefactMargin > 0)
        {
            image = image.copy(QRect(patch->artefactMargin, patch->artefactMargin, rect.width(), rect.height()));
            patch->artefactMargin = 0;
        }

        // Saving PNG is very expensive!
        QBuffer pngBuffer;
        pngBuffer.open(QIODevice::ReadWrite);
        image.save(&pngBuffer, "PNG");
        pngBuffer.close();

        patch->data = pngBuffer.data();
        patch->mimeType = "image/png";
        patch->packedAlpha = false;
    }
    else if (encoderSettings_.patchEncodingFormat == EncoderSettings::EncodingFormat_JPEG || encoderSettings_.patchEncodingFormat & EncoderSettings::EncodingFormat_JPEG)
    {
        if (hasAlphaValues)
            image = ImageAux::createPackedAlphaPatch(image);

        QBuffer jpegBuffer;
        jpegBuffer.open(QIODevice::ReadWrite);
    #ifdef USE_IMPROVED_JPEG
        writeJPEG(image, &jpegBuffer, encoderSettings_.jpegQuality, true, false);
    #else
        image.save(&jpegBuffer, "JPEG", encoderSettings_.jpegQuality);
    #endif
        jpegBuffer.close();

        patch->data = jpegBuffer.data();
        patch->mimeType = "image/jpeg";
        patch->packedAlpha = hasAlphaValues;
    }
    else // if (encoderSettings_.patchEncodingFormat == EncoderSettings::EncodingFormat_Raw)
    {
        if (patch->artefactMargin > 0)
        {
            image = image.copy(QRect(patch->artefactMargin, patch->artefactMargin, rect.width(), rect.height()));
            patch->artefactMargin = 0;
        }

        QBuffer bmpBuffer;
        bmpBuffer.open(QIODevice::ReadWrite);
        image.save(&bmpBuffer, "BMP");
        bmpBuffer.close();

        patch->data = bmpBuffer.data();
        patch->mimeType = "image/bmp";
        patch->packedAlpha = false;
    }

    DPRINTF("rect: %d,%d,%d,%d, %s, image.size: %d kB (%d byte), compressed size: %lld kB (%lld byte)",
            rect.x(), rect.y(), rect.width(), rect.height(), patch->mimeType.constData(),
            image.byteCount() / 1024, image.byteCount(),
            patch->data.size() / 1024, patch->data.size()
            );

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

QList<QByteArray> GraphicsSceneDisplay::takePendingMessages(bool returnBinary)
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

    if (encoderSettings_.useMultithreading)
    {
        QList<UpdateOperation *> updateOps;

        for (int i = 0; i < ops.count(); ++i)
        {
            UpdateOperation *op = &ops[i];
            if (op->type == uotUpdate)
                updateOps.append(op);
        }

        if (updateOps.count() > 0)
            QtConcurrent::blockingFilter(updateOps, GraphicsSceneDisplayThreadedCreatePatch(this));
    }

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

            bool inlineData = patch->data.size() < encoderSettings_.inlineMaxBytes || encoderSettings_.inlineMaxBytes == -1;

            if (inlineData && !returnBinary)
                mimeType = mimeType + ";base64";

            QByteArray msg = QString().sprintf("%s>dI(%d,%d,%d,%d,%d,%d,%s):",
                id_.toLatin1().constData(), frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                patch->artefactMargin,
                mimeType.constData()
            ).toLatin1();

            if (!inlineData)
            {
                QMutexLocker l(&patchesMutex_);
                patch->id = id_ + "_" + QString::number(frame_) + "_" + QString::number(i);

                msg += "fb:" + patch->id;

                patches_.insert(patch->id, patch);
            }
            else
            {
                if (returnBinary)
                    msg += patch->data;
                else
                    msg += patch->toBase64();

                delete patch;
            }

            messageList += msg;
        }
        else if (op.type == uotMove)
        {
            const QRect &rect = op.srcRect;

            QByteArray msg = QString().sprintf("%s>mI(%d,%d,%d,%d,%d,%d,%d):",
                id_.toLatin1().constData(), frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                op.dstPoint.x(), op.dstPoint.y()
            ).toLatin1();

            messageList += msg;
        }
        else if (op.type == uotFill)
        {
            const QRect &rect = op.srcRect;

            QByteArray msg = QString().sprintf("%s>fR(%d,%d,%d,%d,%d,%d,%d,%d,%d):",
                id_.toLatin1().constData(), frame_,
                rect.x(), rect.y(), rect.width(), rect.height(),
                op.fillColor.red(), op.fillColor.green(), op.fillColor.blue(), op.fillColor.alpha()
            ).toLatin1();

            messageList += msg;
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

        if (params.count() == 3)
        {
            int width = params[0].toInt();
            int height = params[1].toInt();
            qreal ratio = params[2].toDouble();

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

