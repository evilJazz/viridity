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

#include "areafingerprint.h"

#include <QThread>
#include <QtConcurrent>

#define VIRIDITY_USE_NSN // Almost 7x faster than naive memcmp
//#define VIRIDITY_USE_NSN_MEMCMP

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

/* AreaFingerPrint */

AreaFingerPrint::AreaFingerPrint() :
    size_(0),
    data_(NULL)
{
}

AreaFingerPrint::AreaFingerPrint(QImage *image, const QRect &area) :
    size_(0),
    data_(NULL)
{
    DGUARDMETHODPROFILED;
    initFromImage(image, area);
}

AreaFingerPrint::AreaFingerPrint(int height) :
    size_(0),
    data_(NULL)
{
    initFromSize(height);
}

AreaFingerPrint::~AreaFingerPrint()
{
    clear();
}

void AreaFingerPrint::clear()
{
    if (data_)
    {
        delete [] data_;
        data_ = NULL;
    }

    size_ = 0;
}

void AreaFingerPrint::initFromSize(int height)
{
    if (height != size_)
    {
        clear();
        size_ = height;
        data_ = new AreaFingerPrintHash[height];
    }
}

void AreaFingerPrint::initFromImage(QImage *image, const QRect &area)
{
    QRect rect = area.intersected(image->rect());
    initFromSize(rect.height());
    internalUpdateFromImage(image, rect, 0);
}

void AreaFingerPrint::updateFromImage(QImage *image, const QRect &area, int startIndex)
{
    if (startIndex >= size_)
        return;

    QRect rect = area.intersected(image->rect());
    internalUpdateFromImage(image, rect, startIndex);
}

void AreaFingerPrint::internalUpdateFromImage(QImage *image, const QRect &rect, int startIndex)
{
    const quint32 *pBuf;

    int limY = qMin(rect.height(), size_ - startIndex);
    int width = rect.width();

    AreaFingerPrintHash hash;

    for (int y = 0; y < limY; ++y)
    {
        pBuf = (const quint32 *)image->constScanLine(rect.top() + y) + rect.left();

        for (int x = width; x > 0; --x)
        {
            hash.add(*pBuf);
            ++pBuf;
        }

        data_[startIndex + y] = hash;
        hash.reset();
    }
}

int AreaFingerPrint::indexOf(const AreaFingerPrint &needle, int startIndex, int endIndex)
{
    int haystackSize = size();
    int needleSize = needle.size();

    if (needleSize > haystackSize || needleSize == 0 || haystackSize == 0)
        return -1;

    if (startIndex > haystackSize)
        return -1;

    int maxIndex = haystackSize - needleSize;
    if (endIndex <= 0 || endIndex > maxIndex)
        endIndex = maxIndex;

    if (endIndex < startIndex)
        return -1;

    const AreaFingerPrintHash *haystackData = constData();
    const AreaFingerPrintHash *needleData = needle.constData();

    if (needleSize > 1)
    {
#ifndef VIRIDITY_USE_NSN
        int needleSizeBytes = needleSize * sizeof(AreaFingerPrintHash);

        int index = startIndex;
        for (; index <= endIndex; ++index)
            if (!memcmp(needleData, &haystackData[index], needleSizeBytes))
                return index;
#else
        //*
        // Based on the Not-So-Naive search algorithm:
        // http://www-igm.univ-mlv.fr/~lecroq/string/
        // Adapted to work on AreaFingerPrintHash

        quint32 index, k, l;
#ifdef VIRIDITY_USE_NSN_MEMCMP
        const size_t needleSizeBytes = (needleSize - 2) * sizeof(AreaFingerPrintHash);
#else
        const int needleSize2 = needleSize - 2;
#endif

        if (needleData[0] == needleData[1])
        {
            k = 2;
            l = 1;
        }
        else
        {
            k = 1;
            l = 2;
        }

        index = startIndex;
        while (index <= endIndex)
        {
            if (needleData[1] != haystackData[index + 1])
            {
                index += k;
            }
            else
            {
#ifdef VIRIDITY_USE_NSN_MEMCMP
                if (needleData[0] == haystackData[index] &&
                    !memcmp(needleData + 2, haystackData + index + 2, needleSizeBytes))
                    return index;
#else
                if (needleData[0] == haystackData[index])
                {
                    int i = 2;
                    while (i < needleSize2)
                    {
                        if (needleData[i] != haystackData[index + i])
                            break;

                        ++i;
                    }

                    if (i == needleSize2)
                        return index;
                }
#endif

                index += l;
            }
        }
#endif
    }
    else
    {
        for (int index = startIndex; index <= endIndex; ++index)
            if (haystackData[index] == needleData[0])
                return index;
    }

    return -1;
}


/* AreaFingerPrints */

AreaFingerPrints::AreaFingerPrints() :
    width_(0),
    height_(0),
    fingerPrints_(NULL)
{
}

AreaFingerPrints::~AreaFingerPrints()
{
    clear();
}

void AreaFingerPrints::clear()
{
    if (fingerPrints_)
    {
        for (int i = 0; i < width_; ++i)
        {
            delete fingerPrints_[i];
            fingerPrints_[i] = NULL;
        }

        delete [] fingerPrints_;
        fingerPrints_ = NULL;
    }

    width_ = 0;
    height_ = 0;
}

void AreaFingerPrints::initFromSize(int width, int height, int templateWidth)
{
    int newWidth = qMax(0, width - templateWidth + 1);

    if (newWidth != width_ || height != height_)
    {
        clear();
        height_ = height;
        width_ = newWidth;
        hashedArea_ = QRect(0, 0, width, height);
        fingerPrints_ = new AreaFingerPrint*[width_];

        for (int i = 0; i < width_; ++i)
            fingerPrints_[i] = new AreaFingerPrint(height_);
    }
}

void AreaFingerPrints::initFromImage(QImage *image, const QRect &area, int templateWidth)
{
    DGUARDMETHODPROFILED;

    QRect rect = area.intersected(image->rect());
    initFromSize(rect.width(), rect.height(), templateWidth);

    hashedArea_ = rect;
    updateFromImage(image, rect);
}

void AreaFingerPrints::updateFromImage(QImage *image, const QRect &area)
{
    QRect rect = hashedArea_.intersected(area.intersected(image->rect()));

    QSize size(templateWidth(), rect.height());

    int leftLimit = qMax(0, rect.left() - templateWidth() + 1);
    //int rightLimit = rect.left() + rect.width() - templateWidth() + 1;
    int rightLimit = qMin(image->width() - templateWidth() + 1, rect.left() + rect.width() - 1);

    int startRow = rect.top() - hashedArea_.top();

    int column = leftLimit - hashedArea_.left();

    for (int x = leftLimit; x < rightLimit; ++x)
    {
        fingerPrints_[column]->updateFromImage(image, QRect(QPoint(x, rect.top()), size), startRow);
        ++column;
    }
}

#ifdef VIRIDITY_USE_MULTITHREADING

struct AreaFingerPrintsThreadedUpdateFromImage
{
    AreaFingerPrintsThreadedUpdateFromImage(QImage *image, AreaFingerPrints *target) :
        image(image), target(target)
    {}

    QImage *image;
    AreaFingerPrints *target;

    bool operator()(const QRect &rect)
    {
        target->updateFromImage(image, rect);
        return true;
    }
};

#endif

void AreaFingerPrints::initFromImageThreaded(QImage *image, const QRect &area, int templateWidth)
{
    DGUARDMETHODPROFILED;

    QRect rect = area.intersected(image->rect());
    initFromSize(rect.width(), rect.height(), templateWidth);

    hashedArea_ = rect;
    updateFromImageThreaded(image, rect);
}

void AreaFingerPrints::updateFromImageThreaded(QImage *image, const QRect &area)
{
    QRect rect = hashedArea_.intersected(area.intersected(image->rect()));

    const int lineHeight = templateWidth() / QThread::idealThreadCount();
    int lineCount = rect.height() / lineHeight;

    if (lineCount == 0)
        updateFromImage(image, rect);
    else
    {
        QVector<QRect> lineRects;

        for (int i = 0; i < lineCount; ++i)
            lineRects.append(QRect(rect.x(), rect.y() + i * lineHeight, rect.width(), lineHeight));

        int partsTotalHeight = lineCount * lineHeight;
        int remainingHeight = rect.height() - partsTotalHeight;
        if (remainingHeight > 0)
            lineRects.append(QRect(rect.x(), rect.y() + partsTotalHeight, rect.width(), remainingHeight));

#ifdef VIRIDITY_USE_MULTITHREADING
        QtConcurrent::blockingFilter(lineRects, AreaFingerPrintsThreadedUpdateFromImage(image, this));
#else
        foreach (const QRect &partRect, lineRects)
            updateFromImage(image, partRect);
#endif
    }
}

bool AreaFingerPrints::findPosition(const AreaFingerPrint &needle, QPoint &result, AreaFingerPrintsPositionMatcher *matcher)
{
    DGUARDMETHODPROFILED;

    for (int line = 0; line < width_; ++line)
    {
        int index = fingerPrints_[line]->indexOf(needle);

        if (index > -1)
        {
            result = QPoint(line, index);
            if (!matcher || (matcher && matcher->positionMatches(result)))
                return true;
        }
    }

    return false;
}

bool AreaFingerPrints::findPosition(const AreaFingerPrint &needle, const QRect &searchArea, QPoint &result, AreaFingerPrintsPositionMatcher *matcher)
{
    DGUARDMETHODPROFILED;

    QRect rect = hashedArea_.intersected(searchArea);

    if (rect.height() < needle.size())
        return false;

    int tempWidth = templateWidth();
    rect.setWidth(rect.width() - tempWidth + 1);
    rect.moveTo(rect.topLeft() - hashedArea_.topLeft());

    int startIndex = rect.top();
    int endIndex = rect.bottom() + 1 - needle.size();

    int colEnd = rect.right() + 1;
    for (int column = rect.left(); column < colEnd; ++column)
    {
        int index = fingerPrints_[column]->indexOf(needle, startIndex, endIndex);

        if (index > -1)
        {
            result = QPoint(column, index);
            if (!matcher || (matcher && matcher->positionMatches(result)))
                return true;
        }
    }

    return false;
}

bool AreaFingerPrints::isEqual(const AreaFingerPrints &other)
{
    if (width() != other.width())
        return false;

    for (int y = 0; y < width(); ++y)
    {
        if (fingerPrints_[y]->size() != other.fingerPrints_[y]->size())
            return false;

        if (memcmp(fingerPrints_[y]->data(), other.fingerPrints_[y]->data(), fingerPrints_[y]->size() * sizeof(AreaFingerPrintHash)) != 0)
            return false;
    }

    return true;
}
