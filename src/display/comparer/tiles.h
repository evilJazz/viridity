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
