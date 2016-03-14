#include "qtquick1adapter.h"

#include <QDeclarativeItem>

QtQuick1Adapter::QtQuick1Adapter(QDeclarativeItem *rootItem) :
    QGraphicsSceneAdapter(rootItem->scene()),
    rootItem_(rootItem)
{
    if (!rootItem_)
        qFatal("Root item is not assigned.");
}

void QtQuick1Adapter::setSize(int width, int height, qreal ratio)
{
    if (!qFuzzyCompare(ratio, 1.f))
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
