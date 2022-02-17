#ifndef VIRIDITYQMLAPPCORE_H
#define VIRIDITYQMLAPPCORE_H

#include <QObject>
#include <QSettings>

#include "viridityqmlwebserver.h"
#include "viriditydeclarative.h"
#include "viridityqmlsessionmanager.h"
#include "viriditydatabridge.h"

#include "handlers/filerequesthandler.h"
#include "handlers/rewriterequesthandler.h"
#include "handlers/debugrequesthandler.h"

class ViridityQmlBasicAppCore : public QObject
{
    Q_OBJECT
public:
    explicit ViridityQmlBasicAppCore(QObject *parent = 0);
    virtual ~ViridityQmlBasicAppCore();

    bool initialize(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl);
    virtual void installTrapsForDefaultQuittingSignals();

    virtual bool startWebServer(const QHostAddress &address, int dataPort);
    virtual void stopWebServer();

    static ViridityQmlBasicAppCore *instance();

protected slots:
    virtual void initEngine();

protected:
    ViridityQmlWebServer *server_;
};

class ViridityQmlExtendedAppCore : public ViridityQmlBasicAppCore
{
    Q_OBJECT
public:
    explicit ViridityQmlExtendedAppCore(QObject *parent = 0);
    virtual ~ViridityQmlExtendedAppCore();

    bool initialize(const QUrl &globalLogicUrl, const QUrl &sessionLogicUrl, const QString &dataLocation);

    QSharedPointer<RewriteRequestHandler> rewriteRequestHandler();

protected:
    virtual void initEngine();

    QString version_;

    QSettings *settings_;

    QString dataLocation_;
    QString settingsFileName_;

    QString debugLogFileName_;

#ifdef VIRIDITY_DEBUG
    QSharedPointer<DebugRequestHandler> debugRequestHandler_;
#endif

    QSharedPointer<RewriteRequestHandler> rewriteRequestHandler_;
};

#endif // VIRIDITYQMLAPPCORE_H
