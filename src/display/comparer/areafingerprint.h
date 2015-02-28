#ifndef AREAFINGERPRINT_H
#define AREAFINGERPRINT_H

#include <QObject>
#include <QImage>

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


class AreaFingerPrint
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

class AreaFingerPrints
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

#endif // AREAFINGERPRINT_H
