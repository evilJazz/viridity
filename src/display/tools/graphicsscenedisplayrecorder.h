#ifndef GRAPHICSSCENEDISPLAYRECORDER_H
#define GRAPHICSSCENEDISPLAYRECORDER_H

#include <QObject>
#include <QFile>
#include <QDataStream>

#include "graphicsscenedisplay.h"

class GraphicsSceneDisplayRecorder : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplayRecorder(GraphicsSceneDisplay *display);
    ~GraphicsSceneDisplayRecorder();

    void setFullFrameFilename(const QString &filename);
    const QString fullFrameFilename() const { return fullFrameFile_.fileName(); }

    void setDiffFrameFilename(const QString &filename);
    const QString diffFrameFilename() const { return diffFrameFile_.fileName(); }

    void setNextFrameTimeStamp(qint64 ts);

private slots:
    void displayNewFrameMessagesGenerated(const QList<QByteArray> &messages);

private:
    GraphicsSceneDisplay *display_;

    qint64 frameTimeStamp_;

    QFile fullFrameFile_;
    QFile diffFrameFile_;

    QDataStream fullFrameData_;
    QDataStream diffFrameData_;
};

#endif // GRAPHICSSCENEDISPLAYRECORDER_H
