#include "viridityqtquickdisplay.h"

#ifdef USE_QTQUICK1
    #include <QGraphicsScene>
    #include <QDeclarativeEngine>
    #include <QDeclarativeContext>
    #include <QDeclarativeComponent>
    #include <QDeclarativeItem>
    typedef QDeclarativeEngine DeclarativeEngine;
    typedef QDeclarativeContext DeclarativeContext;
    typedef QDeclarativeComponent DeclarativeComponent;
    typedef QDeclarativeItem DeclarativeItem;

    #include "display/adapters/qtquick1adapter.h"
#else
    #include <QQmlEngine>
    #include <QQmlContext>
    #include <QQmlComponent>
    #include <QQuickItem>
    #include <QQuickWindow>
    typedef QQmlEngine DeclarativeEngine;
    typedef QQmlContext DeclarativeContext;
    typedef QQmlComponent DeclarativeComponent;
    typedef QQuickItem DeclarativeItem;

    #include "display/adapters/qtquick2adapter.h"
#endif

#include "display/declarativescenesizehandler.h"
#include "display/graphicsscenedisplaymanager.h"

#include "KCL/objectutils.h"
#include "KCL/debug.h"

/* PrivateQtQuickDisplayManager */

class PrivateQtQuickDisplayManager : public AbstractMultiGraphicsSceneDisplayManager
{
    Q_OBJECT
public:
    PrivateQtQuickDisplayManager(ViriditySession *session, ViridityQtQuickDisplay *parent) :
        AbstractMultiGraphicsSceneDisplayManager(session, session),
        parent_(parent)
    {
        DGUARDMETHODTIMED;
    }

    virtual ~PrivateQtQuickDisplayManager()
    {
        DGUARDMETHODTIMED;
    }

protected slots:
    AbstractGraphicsSceneAdapter *getAdapterForItem(QObject *item)
    {
        DGUARDMETHODTIMED;

        AbstractGraphicsSceneAdapter *adapter = NULL;
        QObject *objectToWatch = NULL;

        ObjectUtils::dumpObjectTree(item);

        /*
        // Try to resolve Loader->item...
        if (QString(item->metaObject()->className()).endsWith("Loader"))
        {
            QObject *obj = ObjectUtils::objectify(item->property("item"));

            if (obj)
            {
                ObjectUtils::dumpObjectTree(obj);
                item = obj;
            }
        }
        */

#ifdef USE_QTQUICK1
        QDeclarativeItem *rootItem = qobject_cast<QDeclarativeItem *>(item);

        if (rootItem)
        {
            QGraphicsScene *scene = new QGraphicsScene();
            scene->addItem(rootItem);

            objectToWatch = rootItem;
            QObject::connect(objectToWatch, SIGNAL(destroyed()), scene, SLOT(deleteLater()));

            adapter = new QtQuick1Adapter(rootItem);
        }
#else
        QQuickItem *rootItem = qobject_cast<QQuickItem *>(item);

        if (!rootItem)
        {
            QQuickWindow *window = qobject_cast<QQuickWindow *>(item);
            if (window)
            {
                adapter = new QtQuick2Adapter(window);
                objectToWatch = window;
            }
            else
                adapter = NULL;
        }
        else
        {
            adapter = new QtQuick2Adapter(rootItem);
            objectToWatch = rootItem;
        }
#endif

        if (!adapter)
        {
            qDebug("Could not cast instance to usable type. %s is not supported by ViridityDisplay.", parent_->displayItem()->metaObject()->className());
        }
        else if (objectToWatch)
        {
            if (parent_->autoSize())
                // Install message handler for resize() commands on the item...
                new DeclarativeSceneSizeHandler(session(), parent_->targetId(), adapter, true, objectToWatch);

            QObject::connect(objectToWatch, SIGNAL(destroyed()), adapter, SLOT(deleteLater()));
        }

        return adapter;
    }

    virtual AbstractGraphicsSceneAdapter *getAdapter(const QString &id, const QStringList &params)
    {
        if (id == parent_->targetId() && parent_->displayItem())
        {
            return getAdapterForItem(parent_->displayItem());
        }
        else
        {
            qDebug("No Component or Item contained in ViridityDisplay.");
            return NULL;
        }
    }

private:
    ViridityQtQuickDisplay *parent_;
};


/* ViridityQtQuickDisplay */

ViridityQtQuickDisplay::ViridityQtQuickDisplay(QObject *parent) :
    QObject(parent),
    manager_(NULL),
    displayItem_(NULL),
    targetId_(),
    autoSize_(true)
{
    DGUARDMETHODTIMED;
}

ViridityQtQuickDisplay::~ViridityQtQuickDisplay()
{
    DGUARDMETHODTIMED;
}

void ViridityQtQuickDisplay::classBegin()
{
    DGUARDMETHODTIMED;
}

void ViridityQtQuickDisplay::componentComplete()
{
    DGUARDMETHODTIMED;

    DeclarativeContext *context = DeclarativeEngine::contextForObject(this);

    if (context)
    {
        QObject *obj = ObjectUtils::objectify(context->contextProperty("currentSession"));

        ViriditySession *session = qobject_cast<ViriditySession *>(obj);

        if (session)
            manager_ = new PrivateQtQuickDisplayManager(session, this);

        qDebug("currentSession is %p", session);
    }
    else
        qDebug("Can't determine engine context for object.");
}

QObject *ViridityQtQuickDisplay::displayItem()
{
    return displayItem_;
}

void ViridityQtQuickDisplay::setDisplayItem(QObject *displayItem)
{
    displayItem_ = displayItem;
}

bool ViridityQtQuickDisplay::autoSize() const
{
    return autoSize_;
}

QString ViridityQtQuickDisplay::targetId() const
{
    return targetId_;
}

void ViridityQtQuickDisplay::setAutoSize(bool autoSize)
{
    autoSize_ = autoSize;
}

void ViridityQtQuickDisplay::setTargetId(QString targetId)
{
    targetId_ = targetId;
}

#include "viridityqtquickdisplay.moc"
