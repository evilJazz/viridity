#ifndef GRAPHICSSCENEDISPLAYPLAYER_H
#define GRAPHICSSCENEDISPLAYPLAYER_H

#include <QGraphicsView>
#include <QFile>
#include <QDataStream>
#include <QTimer>
#include <QGraphicsPixmapItem>

class GraphicsSceneDisplayDumpIterator : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplayDumpIterator(QObject *parent = 0);
    virtual ~GraphicsSceneDisplayDumpIterator();

    void setFilename(const QString &filename);
    const QString &filename() const { return inputFile_.fileName(); }

    int advanceToNextFrame();
    const QImage &outputFrame() const { return workBuffer_; }
    qint64 currentFrameTimeStamp() const { return currentTimeStamp_; }

private:
    QFile inputFile_;
    QDataStream inputData_;

    QImage workBuffer_;

    qint64 startTimeStamp_;
    qint64 currentTimeStamp_;
    qint64 lastTimeStamp_;

    enum DumpType {
        Invalid,
        Full,
        Diff
    };

    DumpType type_;
};

class GraphicsSceneDisplayPlayer : public QGraphicsView
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplayPlayer(QWidget *parent = 0);
    virtual ~GraphicsSceneDisplayPlayer();

    void setFilename(const QString &filename);
    const QString &filename() const { return it_.filename(); }

    void play();
    void stop();

private slots:
    void advanceToNextFrame();

private:
    GraphicsSceneDisplayDumpIterator it_;
    QTimer advanceTimer_;
    QGraphicsScene scene_;
    QGraphicsPixmapItem pixmapItem_;
};

#endif // GRAPHICSSCENEDISPLAYPLAYER_H
