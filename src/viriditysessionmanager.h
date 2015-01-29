#ifndef GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
#define GRAPHICSSCENEDISPLAYSESSIONMANAGER_H

#include <QObject>
#include <QElapsedTimer>

#include "graphicsscenedisplay.h"

class ViriditySessionManager;

struct ViriditySession
{
    QString id;

    ViriditySessionManager *sessionManager;
    QGraphicsScene *scene;
    GraphicsSceneDisplay *display;
    GraphicsSceneWebControlCommandInterpreter *commandInterpreter;
    QList<MessageHandler *> commandHandlers;

    QElapsedTimer lastUsed;
    int useCount;
};

class ViriditySessionManager : public QObject
{
    Q_OBJECT
public:
    ViriditySessionManager(QObject *parent = 0);
    virtual ~ViriditySessionManager();

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

    virtual ViriditySession *createNewSessionInstance();
    virtual void setScene(ViriditySession *session) = 0;

protected slots:
    virtual void registerHandlers(ViriditySession *session);
    virtual ViriditySession *createSession(const QString &id) = 0; // Always executed in thread of session manager

private slots:
    void killObsoleteDisplays();

private:
    QMutex displayMutex_;
    QHash<QString, GraphicsSceneDisplay *> displays_;

    QHash<GraphicsSceneDisplay *, ViriditySession *> sessions_;
    QTimer cleanupTimer_;
};

class SingleGraphicsSceneDisplaySessionManager : public ViriditySessionManager
{
    Q_OBJECT
public:
    SingleGraphicsSceneDisplaySessionManager(QObject *parent = 0);
    virtual ~SingleGraphicsSceneDisplaySessionManager();

protected slots:
    virtual ViriditySession *createSession(const QString &id);

private:
    ViriditySession *protoSession_;
};

class MultiGraphicsSceneDisplaySessionManager : public ViriditySessionManager
{
    Q_OBJECT
public:
    MultiGraphicsSceneDisplaySessionManager(QObject *parent = 0);
    virtual ~MultiGraphicsSceneDisplaySessionManager() {}

protected slots:
    virtual ViriditySession *createSession(const QString &id);
};

#endif // GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
