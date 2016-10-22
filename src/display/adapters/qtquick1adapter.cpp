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

#include "qtquick1adapter.h"

#include <QDeclarativeItem>

QtQuick1Adapter::QtQuick1Adapter(QDeclarativeItem *rootItem) :
    QGraphicsSceneAdapter(rootItem->scene()),
    rootItem_(rootItem)
{
    if (!rootItem_)
        qFatal("Root item is not assigned.");

    // Only set focus on root item if we have no other item focused currently...
    if (!rootItem_->focusItem())
        rootItem_->setFocus(true);
}

void QtQuick1Adapter::setSize(int width, int height, qreal ratio)
{
    if (!qFuzzyCompare(ratio, 1.))
    {
        rootItem_->setWidth(width / ratio);
        rootItem_->setHeight(height / ratio);

        rootItem_->setTransformOrigin(QDeclarativeItem::TopLeft);
        rootItem_->setScale(ratio);
    }
    else
    {
        rootItem_->setWidth(width);
        rootItem_->setHeight(height);
    }
}
