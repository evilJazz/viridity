/****************************************************************************
**
** Copyright (C) 2012-2016 Andre Beckedorf, Meteora Softworks
** Contact: info@meteorasoftworks.com
**
** This file is part of Viridity
**
** $VIRIDITY_BEGIN_LICENSE:COMMERCIAL_AGPL$
**
** This library is licensed under either a separately available commercial
** license or the GNU Affero General Public License Version 3.0,
** published 19 November 2007.
**
** See https://www.gnu.org/licenses/agpl-3.0.html or LICENSE-agpl-3.0.txt for
** details.
**
** If you wish to use and distribute the Viridity library in your commercial
** product without making your sourcecode available to the public, please
** contact us for a commercial license at info@meteorasoftworks.com
**
** $VIRIDITY_END_LICENSE$
**
****************************************************************************/

#include "graphicsscenedisplayrecorder.h"

GraphicsSceneDisplayRecorder::GraphicsSceneDisplayRecorder(GraphicsSceneDisplay *display) :
    QObject(display),
    display_(display),
    frameTimeStamp_(0)
{
    connect(display_, SIGNAL(newFrameMessagesGenerated(QList<QByteArray>)), this, SLOT(displayNewFrameMessagesGenerated(QList<QByteArray>)), Qt::DirectConnection);
}

GraphicsSceneDisplayRecorder::~GraphicsSceneDisplayRecorder()
{
}

void GraphicsSceneDisplayRecorder::setFullFrameFilename(const QString &filename)
{
    fullFrameFile_.setFileName(filename);
}

void GraphicsSceneDisplayRecorder::setDiffFrameFilename(const QString &filename)
{
    diffFrameFile_.setFileName(filename);
}

void GraphicsSceneDisplayRecorder::setNextFrameTimeStamp(qint64 ts)
{
    frameTimeStamp_ = ts;
}

void GraphicsSceneDisplayRecorder::displayNewFrameMessagesGenerated(const QList<QByteArray> &messages)
{
    GraphicsSceneDisplayLocker l(display_);

    qint64 frameTimeStamp = frameTimeStamp_ > 0 ? frameTimeStamp_ : QDateTime::currentMSecsSinceEpoch();
    frameTimeStamp_ = 0;

    // Save full frame
    if (!fullFrameFile_.fileName().isEmpty() && !fullFrameFile_.isWritable())
    {
        fullFrameFile_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        fullFrameData_.setDevice(&fullFrameFile_);
        fullFrameData_ << QByteArray("full");
    }

    if (fullFrameFile_.isWritable())
    {
        fullFrameData_ << (qint64)frameTimeStamp;
        fullFrameData_ << display_->renderer().buffer();

        fullFrameFile_.flush();
    }

    // Save differential frame
    if (!diffFrameFile_.fileName().isEmpty() && !diffFrameFile_.isWritable())
    {
        diffFrameFile_.open(QIODevice::WriteOnly | QIODevice::Truncate);
        diffFrameData_.setDevice(&diffFrameFile_);
        diffFrameData_ << QByteArray("diff");
    }

    if (diffFrameFile_.isWritable())
    {
        diffFrameData_ << (qint64)frameTimeStamp;
        diffFrameData_ << display_->renderer().buffer().size();
        diffFrameData_ << messages;
        diffFrameData_ << display_->patches().count();

        QHashIterator<QString, GraphicsSceneFramePatch *> i(display_->patches());
        while (i.hasNext())
        {
            i.next();
            diffFrameData_ << i.key();
            diffFrameData_ << i.value()->data;
        }

        diffFrameFile_.flush();
    }
}

