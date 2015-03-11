#include "graphicsscenedisplayplayer.h"

#include "viriditysessionmanager.h"

#include "KCL/imageutils.h"

/* GraphicsSceneDisplayDumpIterator */

GraphicsSceneDisplayDumpIterator::GraphicsSceneDisplayDumpIterator(QObject *parent) :
    QObject(parent),
    type_(Invalid),
    startTimeStamp_(-1),
    currentTimeStamp_(0),
    lastTimeStamp_(0)
{
}

GraphicsSceneDisplayDumpIterator::~GraphicsSceneDisplayDumpIterator()
{
}

void GraphicsSceneDisplayDumpIterator::setFilename(const QString &filename)
{
    inputFile_.setFileName(filename);

    if (!inputFile_.fileName().isEmpty() && !inputFile_.isReadable())
    {
        type_ = Invalid;
        startTimeStamp_ = -1;

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
                currentTimeStamp_ = startTimeStamp_;
                lastTimeStamp_ = startTimeStamp_;
            }
        }
    }
}

int GraphicsSceneDisplayDumpIterator::advanceToNextFrame()
{
    if (type_ == Invalid)
        return -1;

    currentTimeStamp_ = lastTimeStamp_;

    if (type_ == Full)
    {
        inputData_ >> workBuffer_;
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

        QImage imageBefore = workBuffer_.copy();

        if (bufferSize != workBuffer_.size())
        {
            QImage newImage(bufferSize, QImage::Format_ARGB32);
            newImage.fill(0);

            QPainter p(&newImage);
            p.setBackground(QBrush(QColor(255, 255, 255, 0)));
            p.setCompositionMode(QPainter::CompositionMode_Source);
            p.drawImage(0, 0, workBuffer_);

            workBuffer_ = newImage;
        }

        QPainter p(&workBuffer_);
        p.setBackground(QBrush(QColor(255, 255, 255, 0)));
        p.setCompositionMode(QPainter::CompositionMode_Source);

        for (int i = 0; i < messages.count(); ++i)
        {
            QByteArray message = messages.at(i);
            message = message.mid(message.indexOf('>') + 1);

            QString command;
            QStringList params;

            ViridityMessageHandler::splitMessage(message, command, params);

            if (command == "dI" || command == "drawImage")
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
                    else // raw binary stream...
                    {
                        QByteArray imageData = parts.at(1);
                        image.loadFromData(imageData);
                    }

                    int artefactMargin = params.at(5).toInt();

                    if (mimeType.contains(";pa"))
                    {
                        QImage colorImage(width, height, QImage::Format_ARGB32);
                        colorImage.fill(0);
                        QPainter cp(&colorImage);
                        cp.drawImage(0, 0, image, artefactMargin, artefactMargin);
                        cp.end();

                        QImage alphaImage(width, height, QImage::Format_ARGB32);
                        alphaImage.fill(0);
                        QPainter ap(&alphaImage);
                        ap.drawImage(0, 0, image, artefactMargin, artefactMargin * 3 + height);
                        ap.end();

                        ImageUtils::intensityToAlpha(alphaImage, colorImage);

                        image = colorImage;
                        artefactMargin = 0;
                    }

                    p.drawImage(params.at(1).toInt(), params.at(2).toInt(), image, artefactMargin, artefactMargin);
                }
            }
            else if (command == "fR" || command == "fillRect")
            {
                QBrush b(QColor(params.at(5).toInt(), params.at(6).toInt(), params.at(7).toInt(), params.at(8).toInt()));
                p.fillRect(params.at(1).toInt(), params.at(2).toInt(), params.at(3).toInt(), params.at(4).toInt(), b);
            }
            else if (command == "mI" || command == "moveImage")
            {
                p.drawImage(params.at(5).toInt(), params.at(6).toInt(), imageBefore, params.at(1).toInt(), params.at(2).toInt(), params.at(3).toInt(), params.at(4).toInt());
            }
        }

        p.end();
    }

    if (inputData_.atEnd())
        return 0;

    // Read timestamp of next frame...
    qint64 frameTimeStamp;
    inputData_ >> frameTimeStamp;

    // Calculate diff and start timer...
    qint64 diffTime = frameTimeStamp - lastTimeStamp_;
    lastTimeStamp_ = frameTimeStamp;

    return diffTime;
}

/* GraphicsSceneDisplayPlayer */

GraphicsSceneDisplayPlayer::GraphicsSceneDisplayPlayer(QWidget *parent) :
    QGraphicsView(parent)
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
    it_.setFilename(filename);
}

void GraphicsSceneDisplayPlayer::play()
{
    advanceTimer_.stop();
    advanceToNextFrame();
}

void GraphicsSceneDisplayPlayer::stop()
{
    advanceTimer_.stop();
}

void GraphicsSceneDisplayPlayer::advanceToNextFrame()
{
    int diffTime = it_.advanceToNextFrame();
    pixmapItem_.setPixmap(QPixmap::fromImage(it_.outputFrame()));

    if (diffTime > 0)
    {
        advanceTimer_.setInterval(diffTime);
        advanceTimer_.start();
    }
}

