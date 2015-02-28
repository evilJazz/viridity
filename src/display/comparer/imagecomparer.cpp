#include "imagecomparer.h"

#include "imageaux.h"
#include "moveanalyzer.h"
#include "tiles.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

#define USE_MOVE_ANALYZER
//#define USE_MOVE_ANALYZER_FINEGRAINED
#define USE_FILL_ANALYZER

#ifdef USE_MULTITHREADING
#include <QtConcurrentMap>
#endif

/* ImageComparer */

ImageComparer::ImageComparer(QImage *imageBefore, QImage *imageAfter, int tileWidth) :
    imageBefore_(imageBefore),
    imageAfter_(imageAfter),
    moveAnalyzer_(NULL),
    tileWidth_(tileWidth)
{
#ifdef USE_MOVE_ANALYZER
#ifdef USE_MOVE_ANALYZER_FINEGRAINED
    moveAnalyzer_ = new MoveAnalyzer(imageBefore_, imageAfter_, imageBefore_->rect(), tileWidth_ / 2);
#else
    moveAnalyzer_ = new MoveAnalyzer(imageBefore_, imageAfter_, imageBefore_->rect(), tileWidth_);
#endif
#endif
}

ImageComparer::~ImageComparer()
{
#ifdef USE_MOVE_ANALYZER
    if (moveAnalyzer_)
        delete moveAnalyzer_;
#endif
}

QVector<QRect> ImageComparer::findDifferences()
{
    TiledRegion region(tileWidth_, tileWidth_);

    int width = imageBefore_->width() / tileWidth_ + (imageBefore_->width() % tileWidth_ ? 1 : 0);
    int height = imageBefore_->height() / tileWidth_ + (imageBefore_->height() % tileWidth_ ? 1 : 0);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QRect rect = QRect(x * tileWidth_, y * tileWidth_, tileWidth_, tileWidth_);

            if (!ImageAux::contentMatches<QRgb>(imageBefore_, imageAfter_, rect.topLeft(), rect))
                region += rect;
        }
    }

    return region.rects();
}

#ifdef USE_MULTITHREADING
struct MapProcessRect
{
    MapProcessRect(ImageComparer *comparer, QVector<QRect> *additionalSearchAreas) :
        comparer(comparer),
        additionalSearchAreas(additionalSearchAreas)
    {
    }

    typedef UpdateOperation result_type;

    UpdateOperation operator()(const QRect &rect)
    {
        UpdateOperation op;
        if (!comparer->processRect(rect, op))
            op.type = uotNoOp;

        return op;
    }

    ImageComparer *comparer;
    QVector<QRect> *additionalSearchAreas;
};

void reduceProcessRectToList(UpdateOperationList &list, const UpdateOperation &op)
{
    if (op.type != uotNoOp)
        list.append(op);
}
#endif

UpdateOperationList ImageComparer::findUpdateOperations(const QRect &searchArea, QVector<QRect> *additionalSearchAreas)
{
    DGUARDMETHODTIMED;
    //qDebug("searchArea: %d, %d + %d x %d", searchArea.left(), searchArea.top(), searchArea.width(), searchArea.height());

    UpdateOperationList result;

    QRect minSearchRect = ImageAux::findChangedRect32(imageBefore_, imageAfter_, searchArea);
    if (minSearchRect.isEmpty())
        return result;

    UpdateOperation op;

#if defined(USE_MOVE_ANALYZER) && defined(USE_MOVE_ANALYZER_FINEGRAINED)
    QRegion leftovers;
    MoveOperationList moves = moveAnalyzer_->findMoveOperations(minSearchRect, leftovers, additionalSearchAreas);

    foreach (const MoveOperation &move, moves)
    {
        op.type = uotMove;
        op.srcRect = move.srcRect;
        op.dstPoint = move.dstPoint;
        result.append(op);
    }

    QVector<QRect> searchRects = TiledRegion::verticallyUniteRects(leftovers.rects());

    QList<QRect> tiles;
    foreach (const QRect &rect, searchRects)
        tiles += splitRectIntoTiles(rect, tileWidth_, tileWidth_);
#else

#ifdef USE_MOVE_ANALYZER
    moveAnalyzer_->startNewSearch();
#endif

    QList<QRect> tiles = TileOperations::splitRectIntoTiles(minSearchRect, tileWidth_, tileWidth_);
#endif

#ifdef USE_MULTITHREADING
    result += QtConcurrent::blockingMappedReduced(tiles, MapProcessRect(this, additionalSearchAreas), &reduceProcessRectToList, QtConcurrent::UnorderedReduce);
#else
    foreach (const QRect &rect, tiles)
    {
        if (processRect(rect, op, additionalSearchAreas))
            result.append(op);
    }
#endif

    return result;
}

void ImageComparer::swap()
{
    DGUARDMETHODTIMED;

    QImage *temp = imageBefore_;
    imageBefore_ = imageAfter_;
    imageAfter_ = temp;

#ifdef USE_MOVE_ANALYZER
    moveAnalyzer_->swap();
#endif
}

bool ImageComparer::processRect(const QRect &rect, UpdateOperation &op, QVector<QRect> *additionalSearchAreas)
{
    QRect minRect = ImageAux::findChangedRect32(imageBefore_, imageAfter_, rect);
    if (minRect.isEmpty())
        return false;

#if defined(USE_MOVE_ANALYZER) && !defined(USE_MOVE_ANALYZER_FINEGRAINED)
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
#endif

#ifdef USE_FILL_ANALYZER
    QColor solidColor = ImageAux::getSolidRectColor(imageAfter_, minRect);
    if (solidColor.isValid())
    {
        op.type = uotFill;
        op.srcRect = minRect;
        op.dstPoint = minRect.topLeft();
        op.fillColor = solidColor;

        return true;
    }
#endif

#if defined(USE_MOVE_ANALYZER)
    DPRINTF("No move operation found for: %d, %d + %d x %d",
            minRect.left(), minRect.top(), minRect.width(), minRect.height()
            );
#endif

    op.type = uotUpdate;
    op.srcRect = minRect;
    op.dstPoint = minRect.topLeft();

    return true;
}
