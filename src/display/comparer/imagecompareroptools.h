#ifndef IMAGECOMPAREROPTOOLS_H
#define IMAGECOMPAREROPTOOLS_H

#include <QPoint>
#include <QRect>
#include <QColor>
#include <QList>

enum UpdateOperationType { uotUpdate, uotMove, uotFill, uotNoOp };

struct UpdateOperation
{
    UpdateOperation() : type(uotNoOp), data(NULL) {}
    UpdateOperationType type;
    QRect srcRect;
    QPoint dstPoint;
    QColor fillColor;
    void *data;
};

typedef QList<UpdateOperation> UpdateOperationList;

class ImageComparerOpTools
{
public:
    static UpdateOperationList optimizeUpdateOperations(const UpdateOperationList &ops);
};

#endif // IMAGECOMPAREROPTOOLS_H
