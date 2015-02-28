#include "imagecompareroptools.h"

#include <QRegion>
#include <QVector>

#include <QHash>
#include <QHashIterator>

#include "tiles.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

typedef QHash<QPoint, UpdateOperationList> VectorHashUpdateOperationList;
typedef QHashIterator<QPoint, UpdateOperationList> VectorHashUpdateOperationListIterator;

typedef QHash<QColor, UpdateOperationList> ColorHashUpdateOperationList;
typedef QHashIterator<QColor, UpdateOperationList> ColorHashUpdateOperationListIterator;

inline uint qHash(const QPoint& p)
{
    return qHash(QString().sprintf("%dx%d", p.x(), p.y()));
}

inline uint qHash(const QColor& c)
{
    return qHash((int)c.rgba());
}

UpdateOperationList optimizeVectorizedOperations(UpdateOperationType type, const VectorHashUpdateOperationList &moveOps)
{
    UpdateOperationList newOps;

    VectorHashUpdateOperationListIterator i(moveOps);
    while (i.hasNext())
    {
        i.next();
        const QPoint &vector = i.key();
        const UpdateOperationList &ops = i.value();

        QRegion region;
        foreach (const UpdateOperation &op, ops)
            region += op.srcRect;

        QVector<QRect> rects = TileOperations::verticallyUniteRects(region.rects());

        foreach (const QRect &rect, rects)
        {
            UpdateOperation op;
            op.type = type;
            op.srcRect = rect;
            op.dstPoint = rect.topLeft() - vector;
            newOps.append(op);
        }
    }

    return newOps;
}

UpdateOperationList optimizeFillOperations(const ColorHashUpdateOperationList &fillOps)
{
    UpdateOperationList newOps;

    ColorHashUpdateOperationListIterator i(fillOps);
    while (i.hasNext())
    {
        i.next();

        const QColor &color = i.key();
        const UpdateOperationList &ops = i.value();

        QRegion region;
        foreach (const UpdateOperation &op, ops)
            region += op.srcRect;

        QVector<QRect> rects = TileOperations::verticallyUniteRects(region.rects());

        foreach (const QRect &rect, rects)
        {
            UpdateOperation op;
            op.type = uotFill;
            op.srcRect = rect;
            op.dstPoint = rect.topLeft();
            op.fillColor = color;
            newOps.append(op);
        }
    }

    return newOps;
}

UpdateOperationList ImageComparerOpTools::optimizeUpdateOperations(const UpdateOperationList &ops)
{
    //DGUARDMETHODTIMED;

    VectorHashUpdateOperationList vectorHashedUpdateOps;
    VectorHashUpdateOperationList vectorHashedMoveOps;
    ColorHashUpdateOperationList colorHashedFillOps;

    UpdateOperationList updateOps;
    UpdateOperationList moveOps;
    UpdateOperationList fillOps;

    UpdateOperationList otherOps;

    foreach (const UpdateOperation &op, ops)
    {
        if (op.type == uotUpdate)
        {
            QPoint moveVector(op.srcRect.topLeft() - op.dstPoint);
            vectorHashedUpdateOps[moveVector].append(op);
            updateOps.append(op);
        }
        else if (op.type == uotMove)
        {
            QPoint moveVector(op.srcRect.topLeft() - op.dstPoint);
            vectorHashedMoveOps[moveVector].append(op);
            moveOps.append(op);
        }
        else if (op.type == uotFill)
        {
            colorHashedFillOps[op.fillColor].append(op);
            fillOps.append(op);
        }
        else
            otherOps.append(op);
    }

    UpdateOperationList newUpdateOps = optimizeVectorizedOperations(uotUpdate, vectorHashedUpdateOps);
    UpdateOperationList newMoveOps = optimizeVectorizedOperations(uotMove, vectorHashedMoveOps);
    UpdateOperationList newFillOps = optimizeFillOperations(colorHashedFillOps);

    UpdateOperationList newOps;

    newOps += newUpdateOps.count() < updateOps.count() ? newUpdateOps : updateOps;
    newOps += newFillOps.count() < fillOps.count() ? newFillOps : fillOps;
    newOps += newMoveOps.count() < moveOps.count() ? newMoveOps : moveOps;
    newOps += otherOps;

    DPRINTF("ops: %d, newOps: %d (%d)  moveOps: %d, newMoveOps: %d (%d)  updateOps: %d, newUpdateOps: %d (%d)  fillOps: %d, newFillOps: %d (%d)",
        ops.count(), newOps.count(), ops.count() - newOps.count(),
        moveOps.count(), newMoveOps.count(), moveOps.count() - newMoveOps.count(),
        updateOps.count(), newUpdateOps.count(), updateOps.count() - newUpdateOps.count(),
        fillOps.count(), newFillOps.count(), fillOps.count() - newFillOps.count()
    );

    return newOps;
}
