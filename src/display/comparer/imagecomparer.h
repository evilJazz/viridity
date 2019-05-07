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
        analyzeMoves(false),
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
