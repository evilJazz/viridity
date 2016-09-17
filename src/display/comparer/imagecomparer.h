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
#include <QReadWriteLock>

#include "imagecompareroptools.h"

class MoveAnalyzer;

struct ComparerSettings
{
    ComparerSettings() :
        tileWidth(32),
        useMultithreading(true),
        minifyTiles(true),
        minifyTileCountThreshold(10),
        analyzeFills(true),
        analyzeMoves(true),
        fineGrainedMoves(false)
    {}

    int tileWidth;

    bool useMultithreading;

    bool minifyTiles;
    int minifyTileCountThreshold;

    bool analyzeFills;
    bool analyzeMoves;
    bool fineGrainedMoves;
};

class VIRIDITY_EXPORT ImageComparer
{
public:
    ImageComparer(QImage *imageBefore, QImage *imageAfter);
    virtual ~ImageComparer();

    const ComparerSettings &settings() const { return settings_; }
    void setSettings(const ComparerSettings &settings);

    QVector<QRect> findDifferences();

    UpdateOperationList findUpdateOperations(const QRect &searchArea, QVector<QRect> *additionalSearchAreas = NULL);
    void swap();

private:
    friend class ImageComparerTest;
    QImage *imageBefore_;
    QImage *imageAfter_;

    QReadWriteLock settingsMREW_;

    MoveAnalyzer *moveAnalyzer_;

    ComparerSettings settings_;

    friend struct MapProcessRect;
    bool processRect(const QRect &rect, UpdateOperation &op, QVector<QRect> *additionalSearchAreas = NULL, bool useMinifiedTile = true);
};

#endif // IMAGECOMPARER_H
