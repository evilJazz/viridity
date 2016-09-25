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

#include "tiles.h"

/* TileOperations */

QVector<QRect> TileOperations::verticallyUniteRects(QVector<QRect> rects)
{
    QVector<QRect> result;

    // QRegion::rects outputs always horizontally united rects. However rectangles are not always united vertically due to the nature of the algorithm.
    // Try to find vertically adjacent rectangles with identical width and unite them...

    while (rects.count() > 0)
    {
        QRect inputRect = rects.at(0);
        rects.remove(0);

        bool united = false;

        for (int i = 0; i < result.count(); ++i)
        {
            QRect compareRect = result.at(i);

            //qDebug("compareRect.bottom(): %d, inputRect.top(): %d, compareRect.top(): %d, inputRect.bottom(): %d", compareRect.bottom(), inputRect.top(), compareRect.top(), inputRect.bottom());

            if ((compareRect.bottom() + 1 == inputRect.top() || compareRect.top() == inputRect.bottom() + 1) &&
                (compareRect.left() == inputRect.left() || compareRect.right() == inputRect.right()) &&
                compareRect.width() == inputRect.width())
            {
                result[i] = compareRect.united(inputRect);
                united = true;
            }
        }

        if (!united)
            result.append(inputRect);
    }

    return result;
}

QList<QRect> TileOperations::splitRectIntoTiles(const QRect &rect, int tileWidth, int tileHeight)
{
    //DGUARDMETHODTIMED;

    QList<QRect> tiles;
    int leftX = rect.width() % tileWidth;
    int leftY = rect.height() % tileHeight;

    int y, x;

    int bottomLimit = rect.bottom() + 1 - tileHeight + 1;
    int rightLimit = rect.right() + 1 - tileWidth + 1;

    for (y = rect.top(); y < bottomLimit; y += tileHeight)
    {
        for (x = rect.left(); x < rightLimit; x += tileWidth)
            tiles += QRect(x, y, tileWidth, tileHeight);

        if (leftX > 0)
            tiles += QRect(x, y, leftX, tileHeight);
    }

    if (leftY > 0)
    {
        for (x = rect.left(); x < rightLimit; x += tileWidth)
            tiles += QRect(x, y, tileWidth, leftY);

        if (leftX > 0)
            tiles += QRect(x, y, leftX, leftY);
    }

    return tiles;
}

/* TileRegion */

TiledRegion::TiledRegion(int tileWidth, int tileHeight) :
    tileWidth_(tileWidth),
    tileHeight_(tileHeight)
{

}

TiledRegion::~TiledRegion()
{

}

void TiledRegion::clear()
{
    region_ = QRegion();
}

QVector<QRect> TiledRegion::rects() const
{
    return TileOperations::verticallyUniteRects(region_.rects());
}

TiledRegion &TiledRegion::operator+=(const QRect &r)
{
    QRect newRect;

    newRect.setLeft((r.left() / tileWidth_) * tileWidth_);
    newRect.setTop((r.top() / tileHeight_) * tileHeight_);

    newRect.setRight((r.right() / tileWidth_) * tileWidth_ + (r.right() % tileWidth_ != 0 ? tileWidth_ : 0));
    newRect.setBottom((r.bottom() / tileHeight_) * tileHeight_ + (r.bottom() % tileHeight_ != 0 ? tileHeight_ : 0));

    region_ += newRect;
    return *this;
}

