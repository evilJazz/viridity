#ifndef DECLARATIVESCENESIZEHANDLER_H
#define DECLARATIVESCENESIZEHANDLER_H

#include <QObject>
#include <QtDeclarative>

#include "viriditysessionmanager.h"

class DeclarativeSceneSizeHandler : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    explicit DeclarativeSceneSizeHandler(const QString &id, QDeclarativeItem *rootItem, QObject *parent = 0);
    virtual ~DeclarativeSceneSizeHandler();

    QString id() const { return id_; }

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

    Q_INVOKABLE bool localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

private:
    QString id_;
    QDeclarativeItem *rootItem_;
};

#endif // DECLARATIVESCENESIZEHANDLER_H
