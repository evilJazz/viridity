#ifndef GRAPHICSSCENEDISPLAYPLAYER_H
#define GRAPHICSSCENEDISPLAYPLAYER_H

#include <QGraphicsView>
#include <QFile>
#include <QDataStream>
#include <QTimer>
#include <QGraphicsPixmapItem>

class GraphicsSceneDisplayPlayer : public QGraphicsView
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplayPlayer(QWidget *parent = 0);
    ~GraphicsSceneDisplayPlayer();

    void setFilename(const QString &filename);
    const QString &filename() const { return inputFile_.fileName(); }

    void play();
    void stop();

private slots:
    void advanceToNextFrame();

private:
    QFile inputFile_;
    QDataStream inputData_;

    qint64 startTimeStamp_;
    qint64 lastTimeStamp_;
    QTimer advanceTimer_;

    enum DumpType {
        Invalid,
        Full,
        Diff
    };

    DumpType type_;

    QGraphicsScene scene_;
    QGraphicsPixmapItem pixmapItem_;
};

#endif // GRAPHICSSCENEDISPLAYPLAYER_H
