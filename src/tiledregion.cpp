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
    return region_.rects();
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
