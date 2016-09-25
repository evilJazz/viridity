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
