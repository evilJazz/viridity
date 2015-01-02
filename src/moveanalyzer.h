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
        average_ = (average_ + pixel) / 2;
    }

    void reset()
    {
        hash_ = 0;
        average_ = 0;
    }

    inline bool operator==(const AreaFingerPrintHash &other) const
    {
        return other.average_ == this->average_ &&
               other.hash_ == this->hash_;
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
    void initFromImageSlow(QImage *image, const QRect &area, int templateWidth);
    //void initFromImageFast(QImage *image, const QRect &area, int templateWidth);
    //void initFromImageThreaded(QImage *image, const QRect &area, int templateWidth);

    void updateFromImageSlow(QImage *image, const QRect &area);
    //void updateFromImageFast(QImage *image, const QRect &area);

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

class VIRIDITY_EXPORT MoveAnalyzer : public QObject
{
    Q_OBJECT
public:
    MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, const QRect &hashArea, int templateWidth);
    virtual ~MoveAnalyzer();

    QRect findMovedRect(const QRect &searchArea, const QRect &templateRect);
    QRect findMovedRectAreaFingerPrint(const QRect &searchArea, const QRect &templateRect);
    QRect findMovedRectExhaustive(const QRect &searchArea, const QRect &templateRect);

    void swap();
    void updateArea(const QRect &rect);

    void startNewSearch();
    QRect processRect(const QRect &rect);

signals:
    void changed();

private:
    friend class MoveAnalyzerDebugView;

    QImage *imageBefore_;
    QImage *imageAfter_;
    QRect hashArea_;
    int templateWidth_;
    AreaFingerPrints searchAreaFingerPrints_;
    MoveAnalyzerDebugView *debugView_;

    QList<QRect> damagedAreas_;

    QMutex mutex_;
    int movedRectSearchMisses_;
    bool movedRectSearchEnabled_;
    QList<QPoint> lastSuccessfulMoveVectors_;
};

#endif // MOVEANALYZER_H
