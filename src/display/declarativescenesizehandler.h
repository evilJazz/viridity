#ifndef DECLARATIVESCENESIZEHANDLER_H
#define DECLARATIVESCENESIZEHANDLER_H

#include <QObject>
#include <QtDeclarative>

#include "viriditysessionmanager.h"

class DeclarativeSceneSizeHandler : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    explicit DeclarativeSceneSizeHandler(ViriditySession *session, const QString &id, QDeclarativeItem *rootItem, bool scaleItem = false, QObject *parent = 0);
    virtual ~DeclarativeSceneSizeHandler();

    QString id() const { return id_; }

protected:
    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

    Q_INVOKABLE bool localHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

signals:
    void resized(int width, int height, qreal ratio);

private slots:
    void handleSessionDestroyed();

private:
    ViriditySession *session_;
    QString id_;
    QDeclarativeItem *rootItem_;
    bool scaleItem_;
};

#endif // DECLARATIVESCENESIZEHANDLER_H
