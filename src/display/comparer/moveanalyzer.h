#ifndef MOVEANALYZER_H
#define MOVEANALYZER_H

#include "viridity_global.h"

#include <QObject>
#include <QVector>
#include <QRect>
#include <QImage>
#include <QMutex>

struct MoveOperation
{
    QRect srcRect;
    QPoint dstPoint;
};

typedef QList<MoveOperation> MoveOperationList;

struct VectorEstimate
{
    QPoint vector;
    int extentX;
    int extentY;

    inline bool operator==(const VectorEstimate &other) const
    {
        return other.vector == this->vector &&
               other.extentX == this->extentX &&
               other.extentY == this->extentY;
    }
};

typedef QList<VectorEstimate> VectorEstimates;

#ifdef USE_AREAFINGERPRINTS
class AreaFingerPrints;
#endif

class VIRIDITY_EXPORT MoveAnalyzer : public QObject
{
    Q_OBJECT
public:
    MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, const QRect &hashArea, int templateWidth);
    virtual ~MoveAnalyzer();

    int searchRadius() const { return searchRadius_; }
    void setSearchRadius(int newRadius);

    int searchMissesThreshold() const { return searchMissesThreshold_; }
    void setSearchMissesThreshold(int newThreshold);

    void swap();
    void updateArea(const QRect &rect);

    void startNewSearch();
    QRect processRect(const QRect &rect, QVector<QRect> *additionalSearchAreas = NULL);

    MoveOperationList findMoveOperations(const QRect &rect, QRegion &leftovers, QVector<QRect> *additionalSearchAreas = NULL);

signals:
    void changed();

protected:
    friend class ImageComparerTest;
    QRect findMovedRect(const QRect &searchArea, const QRect &templateRect);
#ifdef USE_AREAFINGERPRINTS
    QRect findMovedRectAreaFingerPrint(const QRect &searchArea, const QRect &templateRect);
#endif
    QRect findMovedRectExhaustive(const QRect &searchArea, const QRect &templateRect);

private:
    friend class MoveAnalyzerAreaFingerPrintsPositionMatcher;

    QImage *imageBefore_;
    QImage *imageAfter_;

    QImage imageBeforeGray_;
    QImage imageAfterGray_;

    QRect hashArea_;
    int templateWidth_;
    int searchRadius_;
#ifdef USE_AREAFINGERPRINTS
    AreaFingerPrints *searchAreaFingerPrints_;
#endif

    QList<QRect> damagedAreas_;

    QMutex mutex_;
    int searchMissesThreshold_;
    int movedRectSearchMisses_;
    bool movedRectSearchEnabled_;
    VectorEstimates lastSuccessfulMoveVectors_;

    void ensureImagesUpdated();
};

#endif // MOVEANALYZER_H