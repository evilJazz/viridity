#include "declarativescenesizehandler.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

DeclarativeSceneSizeHandler::DeclarativeSceneSizeHandler(ViriditySession *session, const QString &id, QDeclarativeItem *rootItem, QObject *parent) :
    QObject(parent),
    session_(session),
    id_(id),
    rootItem_(rootItem)
{
    DGUARDMETHODTIMED;
    if (session_)
    {
        session_->registerMessageHandler(this);
        connect(session_, SIGNAL(destroyed()), this, SLOT(handleSessionDestroyed()), Qt::DirectConnection);
    }
}

DeclarativeSceneSizeHandler::~DeclarativeSceneSizeHandler()
{
    DGUARDMETHODTIMED;
    if (session_)
        session_->unregisterMessageHandler(this);
}

void DeclarativeSceneSizeHandler::handleSessionDestroyed()
{
    session_ = NULL;
}

bool DeclarativeSceneSizeHandler::canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    return targetId == id_ && message.startsWith("resize");
}

bool DeclarativeSceneSizeHandler::handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    bool result;

    QMetaObject::invokeMethod(
        this, "localHandleMessage",
        this->thread() == QThread::currentThread() ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, result),
        Q_ARG(const QByteArray &, message),
        Q_ARG(const QString &, sessionId),
        Q_ARG(const QString &, targetId)
    );

    return result;
}

bool DeclarativeSceneSizeHandler::localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId)
{
    bool result = false;

    QString command;
    QStringList params;

    ViridityMessageHandler::splitMessage(message, command, params);

    if (message.startsWith("resize") && params.count() == 3)
    {
        int width = params[0].toInt();
        int height = params[1].toInt();
        qreal ratio = params[2].toDouble();

        DPRINTF("Received new size: %d x %d, pixel ratio: %f", width, height, ratio);

        if (rootItem_)
        {
            rootItem_->setWidth(width / ratio);
            rootItem_->setHeight(height / ratio);
            rootItem_->setTransformOrigin(QDeclarativeItem::TopLeft);
            rootItem_->setScale(ratio);
        }

        result = true;
    }

    return result;
}
