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

#ifndef MOVEANALYZER_H
#define MOVEANALYZER_H

#include "viridity_global.h"

#include <QObject>
#include <QVector>
#include <QVector2D>
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
    QVector2D vector;
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
    MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, int templateWidth);
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
