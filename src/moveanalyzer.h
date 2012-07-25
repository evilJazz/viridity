#ifndef MOVEANALYZER_H
#define MOVEANALYZER_H

#include "viridity_global.h"

#include <QVector>
#include <QRect>
#include <QImage>

class VIRIDITY_EXPORT AreaFingerPrint
{
public:
    explicit AreaFingerPrint();
    explicit AreaFingerPrint(QImage *image, const QRect &area);
    explicit AreaFingerPrint(int height);
    virtual ~AreaFingerPrint();

    void clear();

    void initFromSize(int height);
    void initFromImage(QImage *image, const QRect &area);

    int size() const { return size_; }
    int indexOf(const AreaFingerPrint &needle, int startIndex = 0, int endIndex = -1);

    inline quint32 *data() { return data_; }
    inline const quint32 *constData() const { return data_; }

private:
    int size_;
    quint32 *data_;
};

class VIRIDITY_EXPORT AreaFingerPrints
{
public:
    explicit AreaFingerPrints();
    virtual ~AreaFingerPrints();

    void clear();

    void initFromSize(int width, int height, int templateWidth);
    void initFromImageSlow(QImage *image, const QRect &area, int templateWidth);
    void initFromImageFast(QImage *image, const QRect &area, int templateWidth);

    void updateFromImage(QImage *image, const QRect &area);
    void initFromImageThreaded(QImage *image, const QRect &area, int templateWidth);

    int width() const { return width_; }
    int height() const { return height_; }
    const QRect &hashedArea() const { return hashedArea_; }
    bool isEqual(const AreaFingerPrints &other);

    int templateWidth() const { return hashedArea_.width() - width_ + 1; }

    bool findPosition(const AreaFingerPrint &needle, QPoint &result);
    bool findPosition(const AreaFingerPrint &needle, const QRect &searchArea, QPoint &result);

private:
    int width_;
    int height_;
    AreaFingerPrint **fingerPrints_;
    QRect hashedArea_;
};

class VIRIDITY_EXPORT MoveAnalyzer
{
public:
    MoveAnalyzer(QImage *imageBefore, QImage *imageAfter, const QRect &hashArea, int templateWidth);

    QRect findMovedRect(const QRect &searchArea, const QRect &templateRect);
    QRect findMovedRectNaive(const QRect &searchArea, const QRect &templateRect);

private:
    QImage *imageBefore_;
    QImage *imageAfter_;
    QRect hashArea_;
    int templateWidth_;
    AreaFingerPrints searchAreaFingerPrints_;
};

#endif // MOVEANALYZER_H
