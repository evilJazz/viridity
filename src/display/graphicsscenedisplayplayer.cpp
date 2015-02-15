#include "graphicsscenedisplayplayer.h"

GraphicsSceneDisplayPlayer::GraphicsSceneDisplayPlayer(QWidget *parent) :
    QGraphicsView(parent),
    type_(Invalid),
    startTimeStamp_(-1)
{
    setScene(&scene_);
    scene_.addItem(&pixmapItem_);

    connect(&advanceTimer_, SIGNAL(timeout()), this, SLOT(advanceToNextFrame()));
    advanceTimer_.setSingleShot(true);
}

GraphicsSceneDisplayPlayer::~GraphicsSceneDisplayPlayer()
{
}

void GraphicsSceneDisplayPlayer::setFilename(const QString &filename)
{
    stop();
    inputFile_.setFileName(filename);
    type_ = Invalid;
    startTimeStamp_ = -1;
}

void GraphicsSceneDisplayPlayer::play()
{
    advanceTimer_.stop();

    if (!inputFile_.fileName().isEmpty() && !inputFile_.isReadable())
    {
        inputFile_.open(QIODevice::ReadOnly);
        inputData_.setDevice(&inputFile_);

        if (inputData_.status() == QDataStream::Ok)
        {
            QByteArray type;
            inputData_ >> type;

            if (type == "full")
                type_ = Full;
            else if (type == "diff")
                type_ = Diff;
            else
                type_ = Invalid;

            if (type_ != Invalid)
            {
                inputData_ >> startTimeStamp_;
                lastTimeStamp_ = startTimeStamp_;
            }
        }
    }

    if (type_ != Invalid)
    {
        advanceToNextFrame();
    }
}

void GraphicsSceneDisplayPlayer::stop()
{
    advanceTimer_.stop();
}

void GraphicsSceneDisplayPlayer::advanceToNextFrame()
{
    if (type_ == Full)
    {
        QImage imageData;
        inputData_ >> imageData;

        QPixmap pixmap = QPixmap::fromImage(imageData);
        pixmapItem_.setPixmap(pixmap);
    }

    if (inputData_.atEnd())
        return;

    // Read timestamp of next frame...
    qint64 frameTimeStamp;
    inputData_ >> frameTimeStamp;

    // Calculate diff and start timer...
    qint64 diffTime = frameTimeStamp - lastTimeStamp_;
    lastTimeStamp_ = frameTimeStamp;
    advanceTimer_.setInterval(diffTime);
    advanceTimer_.start();
}

