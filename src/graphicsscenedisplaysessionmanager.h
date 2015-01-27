#ifndef GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
#define GRAPHICSSCENEDISPLAYSESSIONMANAGER_H

#include <QObject>
#include <QElapsedTimer>

#include "graphicsscenedisplay.h"

class GraphicsSceneDisplaySessionManager;

struct GraphicsSceneDisplaySession
{
    QString id;

    GraphicsSceneDisplaySessionManager *sessionManager;
    QGraphicsScene *scene;
    GraphicsSceneDisplay *display;
    GraphicsSceneWebControlCommandInterpreter *commandInterpreter;
    QList<MessageHandler *> commandHandlers;

    QElapsedTimer lastUsed;
    int useCount;
};

class GraphicsSceneDisplaySessionManager : public QObject
{
    Q_OBJECT
public:
    GraphicsSceneDisplaySessionManager(QObject *parent = 0);
    virtual ~GraphicsSceneDisplaySessionManager();

    GraphicsSceneDisplay *getNewDisplay();

    GraphicsSceneDisplay *getDisplay(const QString &id);

    GraphicsSceneDisplay *acquireDisplay(const QString &id);
    void releaseDisplay(GraphicsSceneDisplay *display);

    QStringList displaysIds(QGraphicsScene *scene = 0);

    int displayCount() const { return displays_.count(); }

signals:
    void newDisplayCreated(GraphicsSceneDisplay *display);
    void displayRemoved(GraphicsSceneDisplay *display);

protected:
    void removeDisplay(GraphicsSceneDisplay *display);

    virtual GraphicsSceneDisplaySession *createNewSessionInstance();
    virtual void setScene(GraphicsSceneDisplaySession *session) = 0;

protected slots:
    virtual void registerHandlers(GraphicsSceneDisplaySession *session);
    virtual GraphicsSceneDisplaySession *createSession(const QString &id) = 0; // Always executed in thread of session manager

private slots:
    void killObsoleteDisplays();

private:
    QMutex displayMutex_;
    QHash<QString, GraphicsSceneDisplay *> displays_;

    QHash<GraphicsSceneDisplay *, GraphicsSceneDisplaySession *> sessions_;
    QTimer cleanupTimer_;
};

class SingleGraphicsSceneDisplaySessionManager : public GraphicsSceneDisplaySessionManager
{
    Q_OBJECT
public:
    SingleGraphicsSceneDisplaySessionManager(QObject *parent = 0);
    virtual ~SingleGraphicsSceneDisplaySessionManager();

protected slots:
    virtual GraphicsSceneDisplaySession *createSession(const QString &id);

private:
    GraphicsSceneDisplaySession *protoSession_;
};

class MultiGraphicsSceneDisplaySessionManager : public GraphicsSceneDisplaySessionManager
{
    Q_OBJECT
public:
    MultiGraphicsSceneDisplaySessionManager(QObject *parent = 0);
    virtual ~MultiGraphicsSceneDisplaySessionManager() {}

protected slots:
    virtual GraphicsSceneDisplaySession *createSession(const QString &id);
};

#endif // GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
