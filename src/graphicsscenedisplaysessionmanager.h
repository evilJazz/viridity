#ifndef GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
#define GRAPHICSSCENEDISPLAYSESSIONMANAGER_H

#include <QObject>
#include <QElapsedTimer>

#include "graphicsscenedisplay.h"

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

    int displayCount() const { return displays_.count(); }

signals:
    void newDisplayCreated(GraphicsSceneDisplay *display);
    void displayRemoved(GraphicsSceneDisplay *display);

protected:
    void removeDisplay(GraphicsSceneDisplay *display);

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id) = 0; // Always executed in thread of session manager

private slots:
    void killObsoleteDisplays();

private:
    QMutex displayMutex_;
    QHash<QString, GraphicsSceneDisplay *> displays_;

    struct DisplayResource {
        GraphicsSceneDisplay *display;
        QElapsedTimer lastUsed;
        int useCount;
    };

    QHash<GraphicsSceneDisplay *, DisplayResource> displayResources_;
    QTimer cleanupTimer_;
};

class SingleGraphicsSceneDisplaySessionManager : public GraphicsSceneDisplaySessionManager
{
    Q_OBJECT
public:
    SingleGraphicsSceneDisplaySessionManager(QObject *parent, QGraphicsScene *scene);
    virtual ~SingleGraphicsSceneDisplaySessionManager() {}

    QGraphicsScene *scene() const { return scene_; }
    GraphicsSceneWebControlCommandInterpreter *commandInterpreter() const { return commandInterpreter_; }

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id);

private:
    QGraphicsScene *scene_;
    GraphicsSceneWebControlCommandInterpreter *commandInterpreter_;
};

class MultiGraphicsSceneDisplaySessionManager : public GraphicsSceneDisplaySessionManager
{
    Q_OBJECT
public:
    MultiGraphicsSceneDisplaySessionManager(QObject *parent = 0);
    virtual ~MultiGraphicsSceneDisplaySessionManager() {}

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id);
    virtual QGraphicsScene *getScene(const QString &id) = 0;
};

#endif // GRAPHICSSCENEDISPLAYSESSIONMANAGER_H
