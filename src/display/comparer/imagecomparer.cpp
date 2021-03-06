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

#include "imagecomparer.h"

#include "imageaux.h"
#include "moveanalyzer.h"
#include "tiles.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

#include <QtConcurrentMap>

/* ImageComparer */

ImageComparer::ImageComparer(QImage *imageBefore, QImage *imageAfter) :
    imageBefore_(imageBefore),
    imageAfter_(imageAfter),
    moveAnalyzer_(NULL),
    settingsMREW_(QReadWriteLock::Recursive)
{
    setSettings(ComparerSettings());
}

ImageComparer::~ImageComparer()
{
    if (moveAnalyzer_)
        delete moveAnalyzer_;
}

void ImageComparer::setSettings(const ComparerSettings &settings)
{
    QWriteLocker l(&settingsMREW_);

    settings_ = settings;

    if (moveAnalyzer_)
        delete moveAnalyzer_;

    int moveTileWidth = settings_.fineGrainedMoves ? settings_.tileWidth / 2 : settings_.tileWidth;
    moveAnalyzer_ = new MoveAnalyzer(imageBefore_, imageAfter_, moveTileWidth);
}

QVector<QRect> ImageComparer::findDifferences()
{
    QReadLocker l(&settingsMREW_);

    TiledRegion region(settings_.tileWidth, settings_.tileWidth);

    int width = imageBefore_->width() / settings_.tileWidth + (imageBefore_->width() % settings_.tileWidth ? 1 : 0);
    int height = imageBefore_->height() / settings_.tileWidth + (imageBefore_->height() % settings_.tileWidth ? 1 : 0);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QRect rect = QRect(x * settings_.tileWidth, y * settings_.tileWidth, settings_.tileWidth, settings_.tileWidth);

            if (!ImageAux::contentMatches<QRgb>(imageBefore_, imageAfter_, rect.topLeft(), rect))
                region += rect;
        }
    }

    return region.rects();
}

struct MapProcessRect
{
    MapProcessRect(ImageComparer *comparer, QVector<QRect> *additionalSearchAreas, bool useMinifiedTile) :
        comparer(comparer),
        additionalSearchAreas(additionalSearchAreas),
        useMinifiedTile(useMinifiedTile)
    {
    }

    typedef UpdateOperation result_type;

    UpdateOperation operator()(const QRect &rect)
    {
        UpdateOperation op;
        if (!comparer->processRect(rect, op, additionalSearchAreas, useMinifiedTile))
            op.type = uotNoOp;

        return op;
    }

    ImageComparer *comparer;
    QVector<QRect> *additionalSearchAreas;
    bool useMinifiedTile;
};

void reduceProcessRectToList(UpdateOperationList &list, const UpdateOperation &op)
{
    if (op.type != uotNoOp)
        list.append(op);
}

UpdateOperationList ImageComparer::findUpdateOperations(const QRect &searchArea, QVector<QRect> *additionalSearchAreas)
{
    DGUARDMETHODTIMED;
    QReadLocker l(&settingsMREW_);

    //qDebug("searchArea: %d, %d + %d x %d", searchArea.left(), searchArea.top(), searchArea.width(), searchArea.height());

    UpdateOperationList result;

    QRect minSearchRect = ImageAux::findChangedRect32(imageBefore_, imageAfter_, searchArea);
    if (minSearchRect.isEmpty())
        return result;

    UpdateOperation op;

    QList<QRect> tiles;

    if (settings_.analyzeMoves && settings_.fineGrainedMoves)
    {
        QRegion leftovers;
        MoveOperationList moves = moveAnalyzer_->findMoveOperations(minSearchRect, leftovers, additionalSearchAreas);

        foreach (const MoveOperation &move, moves)
        {
            op.type = uotMove;
            op.srcRect = move.srcRect;
            op.dstPoint = move.dstPoint;
            result.append(op);
        }

        QVector<QRect> searchRects = TileOperations::verticallyUniteRects(leftovers.rects());

        foreach (const QRect &rect, searchRects)
            tiles += TileOperations::splitRectIntoTiles(rect, settings_.tileWidth, settings_.tileWidth);
    }
    else
    {
        if (settings_.analyzeMoves)
            moveAnalyzer_->startNewSearch();

        tiles = TileOperations::splitRectIntoTiles(minSearchRect, settings_.tileWidth, settings_.tileWidth);
    }

    bool useMinifiedTile = settings_.minifyTiles && (settings_.minifyTileCountThreshold == -1 || tiles.count() <= settings_.minifyTileCountThreshold);

    if (settings_.useMultithreading)
    {
        result += QtConcurrent::blockingMappedReduced(tiles, MapProcessRect(this, additionalSearchAreas, useMinifiedTile), &reduceProcessRectToList, QtConcurrent::UnorderedReduce);
    }
    else
    {
        foreach (const QRect &rect, tiles)
        {
            if (processRect(rect, op, additionalSearchAreas, useMinifiedTile))
                result.append(op);
        }
    }

    return result;
}

void ImageComparer::swap()
{
    DGUARDMETHODTIMED;
    QReadLocker l(&settingsMREW_);

    QImage *temp = imageBefore_;
    imageBefore_ = imageAfter_;
    imageAfter_ = temp;

    if (settings_.analyzeMoves)
        moveAnalyzer_->swap();
}

bool ImageComparer::processRect(const QRect &rect, UpdateOperation &op, QVector<QRect> *additionalSearchAreas, bool useMinifiedTile)
{
    QReadLocker l(&settingsMREW_);

    QRect minRect = ImageAux::findChangedRect32(imageBefore_, imageAfter_, rect);
    if (minRect.isEmpty())
        return false;

    if (!useMinifiedTile)
        minRect = rect;

    if (settings_.analyzeFills)
    {
        QColor solidColor = ImageAux::getSolidRectColor(imageAfter_, minRect);
        if (solidColor.isValid())
        {
            op.type = uotFill;
            op.srcRect = minRect;
            op.dstPoint = minRect.topLeft();
            op.fillColor = solidColor;

            return true;
        }
    }

    if (settings_.analyzeMoves && !settings_.fineGrainedMoves)
    {
        QRect movedSrcRect = moveAnalyzer_->processRect(rect);
        if (!movedSrcRect.isEmpty())
        {
            op.type = uotMove;
            op.srcRect = movedSrcRect;
            op.dstPoint = rect.topLeft();

            DPRINTF("Move  %d, %d + %d x %d  ->  %d, %d + %d x %d",
                    movedSrcRect.left(), movedSrcRect.top(), rect.width(), rect.height(),
                    rect.left(), rect.top(), rect.width(), rect.height()
                    );

            return true;
        }
    }

    if (settings_.analyzeMoves && !settings_.fineGrainedMoves)
    {
        DPRINTF("No move operation found for: %d, %d + %d x %d",
                minRect.left(), minRect.top(), minRect.width(), minRect.height()
                );
    }

    op.type = uotUpdate;
    op.srcRect = minRect;
    op.dstPoint = minRect.topLeft();

    return true;
}
