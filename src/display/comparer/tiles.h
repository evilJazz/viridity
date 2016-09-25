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

#ifndef TILES_H
#define TILES_H

#include <QRegion>
#include <QVector>
#include <QRect>

class TileOperations
{
public:
    static QVector<QRect> verticallyUniteRects(QVector<QRect> rects);
    static QList<QRect> splitRectIntoTiles(const QRect &rect, int tileWidth, int tileHeight);
};

class TiledRegion
{
public:
    TiledRegion(int tileWidth = 20, int tileHeight = 20);
    ~TiledRegion();

    void clear();
    QVector<QRect> rects() const;

    TiledRegion &operator+=(const QRect &r);

private:
    QRegion region_;
    int tileWidth_;
    int tileHeight_;
};

#endif // TILES_H
