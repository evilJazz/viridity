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
    id_ = createUniqueID();

    renderer_ = new GraphicsSceneBufferRenderer();
    renderer_->setTargetGraphicsScene(server_->scene());

    connect(renderer_, SIGNAL(damagedRegionAvailable()), this, SLOT(sceneDamagedRegionsAvailable()));

    connect(&timer_, SIGNAL(timeout()), this, SLOT(sendUpdate()));
    timer_.setSingleShot(false);
    timer_.start(updateCheckInterval_);

    renderer_->setEnabled(true);
}

GraphicsSceneDisplay::~GraphicsSceneDisplay()
{
    qDeleteAll(patches_.values());

    server_->removeDisplay(this);
}

Patch *GraphicsSceneDisplay::takePatch(const QString &patchId)
{
    QMutexLocker l(&patchesMutex_);

    if (patches_.contains(patchId))
        return patches_.take(patchId);
    else
        return NULL;
}

bool GraphicsSceneDisplay::sendCommand(const QByteArray &data)
{
    QString rawCommand = data;

    int paramStartIndex = rawCommand.indexOf("(");
    int paramStopIndex = rawCommand.indexOf(")");

    QString command = rawCommand.mid(0, paramStartIndex);
    QString rawParams = rawCommand.mid(paramStartIndex + 1, paramStopIndex - paramStartIndex - 1);

    QStringList params = rawParams.split(",", QString::KeepEmptyParts);

    //DPRINTF("%p -> received message: %s, command: %s, rawParams: %s", socket_, data.constData(), command.toLatin1().constData(), rawParams.toLatin1().constData());

    if (command != "ready")
        sendCommand(command, params);
    else
        clientReady();
}

bool GraphicsSceneDisplay::sendCommand(const QString &command, const QStringList &params)
{
    bool result = true;

    if (command.startsWith("requestFullUpdate"))
        renderer_->fullUpdate();
    else
        metaObject()->invokeMethod(
            server_->commandInterpreter(), "sendCommand",
            server_->scene()->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, result),
            Q_ARG(const QString &, command),
            Q_ARG(const QStringList &, params)
        );

    return result;
}

void GraphicsSceneDisplay::clientReady()
{
    clientReady_ = true;
}

void GraphicsSceneDisplay::sceneDamagedRegionsAvailable()
{
    if (!timer_.isActive())
        timer_.start(updateCheckInterval_);
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
    //image.save(&patch->data, "JPEG", 90);
    //image.save(&patch->data, "BMP");
    image.save(&patch->data, "PNG");
    patch->data.close();

    patch->mimeType = "image/png";

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

    DPRINTF("connection: %p  thread: %p  clientReady_: %d  patches_.count(): %d", this, QThread::currentThread(), clientReady_, patches_.count());
    timer_.stop();

    if (clientReady_ && patches_.count() == 0)
    {
        l.unlock();
        updateAvailable_ = true;
        emit updateAvailable();
    }
}

QStringList GraphicsSceneDisplay::getUpdateCommandList()
{
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
                Patch *patch = createPatch(rect, true);

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
