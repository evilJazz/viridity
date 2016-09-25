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
    const QString filename() const { return inputFile_.fileName(); }

    int advanceToNextFrame();
    const QImage &outputFrame() const { return workBuffer_; }
    qint64 currentFrameTimeStamp() const { return currentTimeStamp_; }

private:
    enum DumpType {
        Invalid,
        Full,
        Diff
    };

    DumpType type_;

    QFile inputFile_;
    QDataStream inputData_;

    QImage workBuffer_;

    qint64 startTimeStamp_;
    qint64 currentTimeStamp_;
    qint64 lastTimeStamp_;
};

class GraphicsSceneDisplayPlayer : public QGraphicsView
{
    Q_OBJECT
public:
    explicit GraphicsSceneDisplayPlayer(QWidget *parent = 0);
    virtual ~GraphicsSceneDisplayPlayer();

    void setFilename(const QString &filename);
    const QString filename() const { return it_.filename(); }

    void play();
    void stop();

private slots:
    void advanceToNextFrame();

private:
    GraphicsSceneDisplayDumpIterator it_;
    QTimer *advanceTimer_;
    QGraphicsScene scene_;
    QGraphicsPixmapItem pixmapItem_;
};

#endif // GRAPHICSSCENEDISPLAYPLAYER_H
