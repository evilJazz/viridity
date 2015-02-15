#include "graphicsscenedisplayplayer.h"

#include "viriditysessionmanager.h"

#include "KCL/imageutils.h"

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
    else if (type_ == Diff)
    {
        QSize bufferSize;
        inputData_ >> bufferSize;

        QList<QByteArray> messages;
        inputData_ >> messages;

        int patchCount;
        inputData_ >> patchCount;

        QHash<QString, QByteArray> patches;
        for (int i = 0; i < patchCount; ++i)
        {
            QString patchId;
            QByteArray imageData;

            inputData_ >> patchId;
            inputData_ >> imageData;

            patches.insert(patchId, imageData);
        }

        // Interpret data...
        QPixmap pixmap(bufferSize);

        QPainter p(&pixmap);
        p.drawPixmap(0, 0, pixmapItem_.pixmap());

        for (int i = 0; i < messages.count(); ++i)
        {
            QByteArray message = messages.at(i);
            message = message.mid(message.indexOf('>') + 1);

            QString command;
            QStringList params;

            ViridityMessageHandler::splitMessage(message, command, params);

            if (command == "drawImage")
            {
                QList<QByteArray> parts = message.split(':');

                if (parts.count() > 1)
                {
                    QImage image;

                    int width = params.at(3).toInt();
                    int height = params.at(4).toInt();

                    QString mimeType = params.at(6);

                    if (parts.at(1).startsWith("fb") && parts.count() > 2)
                    {
                        image.loadFromData(patches.value(parts.at(2)));
                    }
                    else if (mimeType.contains(";base64"))
                    {
                        QByteArray imageData = QByteArray::fromBase64(parts.at(1));
                        image.loadFromData(imageData);
                    }

                    if (mimeType.contains(";pa"))
                    {
                        int artefactMargin = params.at(5).toInt();

                        QImage colorImage(width, height, QImage::Format_ARGB32);
                        QPainter cp(&colorImage);
                        cp.drawImage(0, 0, image, artefactMargin, artefactMargin);
                        cp.end();

                        QImage alphaImage(width, height, QImage::Format_ARGB32);
                        QPainter ap(&alphaImage);
                        ap.drawImage(0, 0, image, artefactMargin, artefactMargin * 2 + height);
                        ap.end();

                        ImageUtils::intensityToAlpha(alphaImage, colorImage);

                        image = colorImage;
                    }

                    p.drawImage(params.at(1).toInt(), params.at(2).toInt(), image);
                }
            }
            else if (command == "fillRect")
            {
                QBrush b(QColor(params.at(5).toInt(), params.at(6).toInt(), params.at(7).toInt(), params.at(8).toInt()));
                p.fillRect(params.at(1).toInt(), params.at(2).toInt(), params.at(3).toInt(), params.at(4).toInt(), b);
            }
            else if (command == "moveRect")
            {

            }
        }

        p.end();

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

