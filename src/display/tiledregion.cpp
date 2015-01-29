#include "tiledregion.h"

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
    return TiledRegion::verticallyUniteRects(region_.rects());
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

QVector<QRect> TiledRegion::verticallyUniteRects(QVector<QRect> rects)
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
