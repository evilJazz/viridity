#ifndef TILEDREGION_H
#define TILEDREGION_H

#include <QRegion>
#include <QVector>
#include <QRect>

class TiledRegion
{
public:
    TiledRegion(int tileWidth = 20, int tileHeight = 20);
    ~TiledRegion();

    void clear();
    QVector<QRect> rects() const;

    TiledRegion &operator+=(const QRect &r);

    static QVector<QRect> verticallyUniteRects(QVector<QRect> rects);

private:
    QRegion region_;
    int tileWidth_;
    int tileHeight_;
};

#endif // TILEDREGION_H
