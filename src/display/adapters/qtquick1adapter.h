#ifndef QTQUICK1ADAPTER_H
#define QTQUICK1ADAPTER_H

#include "qgraphicssceneadapter.h"

class QDeclarativeItem;

class QtQuick1Adapter : public QGraphicsSceneAdapter
{
public:
    QtQuick1Adapter(QDeclarativeItem *rootItem);

    virtual void setSize(int width, int height, qreal ratio);

private:
    QDeclarativeItem *rootItem_;
};

#endif // QTQUICK1ADAPTER_H
