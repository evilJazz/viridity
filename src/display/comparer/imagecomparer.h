#ifndef IMAGECOMPARER_H
#define IMAGECOMPARER_H

#include "viridity_global.h"

#include <QRegion>
#include <QVector>
#include <QList>
#include <QHash>
#include <QHashIterator>
#include <QRect>
#include <QColor>
#include <QImage>

#include "imagecompareroptools.h"

class MoveAnalyzer;

class VIRIDITY_EXPORT ImageComparer
{
public:
    ImageComparer(QImage *imageBefore, QImage *imageAfter, int tileWidth = 32);
    virtual ~ImageComparer();

    int tileSize() const { return tileWidth_; }

    QVector<QRect> findDifferences();

    UpdateOperationList findUpdateOperations(const QRect &searchArea, QVector<QRect> *additionalSearchAreas = NULL);
    void swap();

protected:
    friend struct MapProcessRect;
    bool processRect(const QRect &rect, UpdateOperation &op, QVector<QRect> *additionalSearchAreas = NULL);

private:
    friend class ImageComparerTest;
    QImage *imageBefore_;
    QImage *imageAfter_;

    MoveAnalyzer *moveAnalyzer_;
    int tileWidth_;
};

#endif // IMAGECOMPARER_H
