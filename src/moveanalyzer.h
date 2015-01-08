#ifndef MOVEANALYZER_H
#define MOVEANALYZER_H

#include "viridity_global.h"

#include <QObject>
#include <QVector>
#include <QRect>
#include <QImage>
#include <QMutex>

struct AreaFingerPrintHash
{
    AreaFingerPrintHash() : hash_(0), average_(0) {}

    inline void add(const quint32 &pixel)
    {
        hash_ += pixel;
        average_ = (average_ + pixel) >> 1;
    }

    inline void reset()
    {
        hash_ = 0;
        average_ = 0;
    }

    inline bool operator==(const AreaFingerPrintHash &other) const
    {
        return other.average_ == this->average_ &&
               other.hash_ == this->hash_;
    }

    inline bool operator!=(const AreaFingerPrintHash &other) const
    {
        return !(other == *this);
    }

    quint32 hash_;
    quint32 average_;
};


class VIRIDITY_EXPORT AreaFingerPrint
{
public:
    explicit AreaFingerPrint();
    explicit AreaFingerPrint(QImage *image, const QRect &area);
    explicit AreaFingerPrint(int height);
    ~AreaFingerPrint();

    void clear();

    void initFromSize(int height);
    void initFromImage(QImage *image, const QRect &area);

    void updateFromImage(QImage *image, const QRect &area, int startIndex = 0);

    int size() const { return size_; }
    int indexOf(const AreaFingerPrint &needle, int startIndex = 0, int endIndex = -1);

    inline AreaFingerPrintHash *data() { return data_; }
    inline const AreaFingerPrintHash *constData() const { return data_; }

private:
    int size_;
    AreaFingerPrintHash *data_;

    void internalUpdateFromImage(QImage *image, const QRect &rect, int startIndex);
};

struct AreaFingerPrintsPositionMatcher
{
    virtual bool positionMatches(const QPoint &pos) = 0;
};

class VIRIDITY_EXPORT AreaFingerPrints
{
public:
    explicit AreaFingerPrints();
    ~AreaFingerPrints();

    void clear();

    void initFromSize(int width, int height, int templateWidth);
    void initFromImage(QImage *image, const QRect &area, int templateWidth);
    void initFromImageThreaded(QImage *image, const QRect &area, int templateWidth);

    void updateFromImage(QImage *image, const QRect &area);
    void updateFromImageThreaded(QImage *image, const QRect &area);

    int width() const { return width_; }
    int height() const { return height_; }
    const QRect &hashedArea() const { return hashedArea_; }
    bool isEqual(const AreaFingerPrints &other);

    AreaFingerPrint **fingerPrints() const { return fingerPrints_; }

    int templateWidth() const { return hashedArea_.width() - width_ + 1; }

    bool findPosition(const AreaFingerPrint &needle, QPoint &result, AreaFingerPrintsPositionMatcher *matcher = 0);
    bool findPosition(const AreaFingerPrint &needle, const QRect &searchArea, QPoint &result, AreaFingerPrintsPositionMatcher *matcher = 0);

private:
    int width_;
    int height_;
    AreaFingerPrint **fingerPrints_;
    QRect hashedArea_;
};

class MoveAnalyzerDebugView;

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
    QRect findMovedRectAreaFingerPrint(const QRect &searchArea, const QRect &templateRect);
    QRect findMovedRectExhaustive(const QRect &searchArea, const QRect &templateRect);

private:
    friend class MoveAnalyzerDebugView;

    QImage *imageBefore_;
    QImage *imageAfter_;

    QImage imageBeforeGray_;
    QImage imageAfterGray_;

    QRect hashArea_;
    int templateWidth_;
    int searchRadius_;
    AreaFingerPrints searchAreaFingerPrints_;
    MoveAnalyzerDebugView *debugView_;

    QList<QRect> damagedAreas_;

    QMutex mutex_;
    int searchMissesThreshold_;
    int movedRectSearchMisses_;
    bool movedRectSearchEnabled_;
    VectorEstimates lastSuccessfulMoveVectors_;

    void ensureImagesUpdated();
};

#endif // MOVEANALYZER_H
