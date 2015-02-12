#ifndef GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
#define GRAPHICSSCENEDISPLAYSESSIONMANAGER_H

#include <QObject>
#include <QElapsedTimer>

#include "viriditysessionmanager.h"
#include "graphicsscenedisplay.h"
#include "handlers/patchrequesthandler.h"

class MainThreadGateway;

class GraphicsSceneDisplaySessionManager : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    GraphicsSceneDisplaySessionManager(ViriditySession *session, QObject *parent = 0);
    virtual ~GraphicsSceneDisplaySessionManager();

    GraphicsSceneDisplay *getNewDisplay(const QString &id, const QStringList &params);

    GraphicsSceneDisplay *getDisplay(const QString &id);

    GraphicsSceneDisplay *acquireDisplay(const QString &id);
    void releaseDisplay(GraphicsSceneDisplay *display);

    int displayCount() const { return displays_.count(); }

    static QList<GraphicsSceneDisplaySessionManager *> activeSessionManagers();

    ViriditySession *session() const { return session_; }

signals:
    void newDisplayCreated(GraphicsSceneDisplay *display);
    void displayRemoved(GraphicsSceneDisplay *display);

protected:
    void removeDisplay(GraphicsSceneDisplay *display);

    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

protected slots:
    friend class MainThreadGateway;
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params) = 0; // Always executed in thread of qApp
    virtual void tearDownDisplayInstance(GraphicsSceneDisplay *display); // Always executed in thread of qApp

private slots:
    void killObsoleteDisplays();
    void killAllDisplays();

    void handleDisplayUpdateAvailable();
    void handleSessionDestroyed();

private:
    ViriditySession *session_;
    QMutex displayMutex_;
    QHash<QString, GraphicsSceneDisplay *> displays_;

    struct DisplayResource {
        GraphicsSceneDisplay *display;
        QElapsedTimer lastUsed;
        int useCount;
    };

    QHash<GraphicsSceneDisplay *, DisplayResource> displayResources_;
    QTimer cleanupTimer_;

    static QList<GraphicsSceneDisplaySessionManager *> activeSessionManagers_;

    PatchRequestHandler *patchRequestHandler_;

    MainThreadGateway *mainThreadGateway_;
};

class SingleGraphicsSceneDisplaySessionManager : public GraphicsSceneDisplaySessionManager
{
    Q_OBJECT
public:
    SingleGraphicsSceneDisplaySessionManager(ViriditySession *session, QObject *parent, QGraphicsScene *scene);
    virtual ~SingleGraphicsSceneDisplaySessionManager() {}

    QGraphicsScene *scene() const { return scene_; }
    GraphicsSceneWebControlCommandInterpreter *commandInterpreter() const { return commandInterpreter_; }

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params);

private:
    QGraphicsScene *scene_;
    GraphicsSceneWebControlCommandInterpreter *commandInterpreter_;
};

class MultiGraphicsSceneDisplaySessionManager : public GraphicsSceneDisplaySessionManager
{
    Q_OBJECT
public:
    MultiGraphicsSceneDisplaySessionManager(ViriditySession *session, QObject *parent = 0);
    virtual ~MultiGraphicsSceneDisplaySessionManager() {}

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params);
    virtual void tearDownDisplayInstance(GraphicsSceneDisplay *display);

    virtual QGraphicsScene *getScene(const QString &id, const QStringList &params) = 0;
    virtual void tearDownScene(const QString &id, QGraphicsScene *scene);
};

#endif // GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
