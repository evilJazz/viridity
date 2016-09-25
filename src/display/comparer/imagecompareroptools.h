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
