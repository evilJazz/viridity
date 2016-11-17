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

#ifndef GRAPHICSSCENEDISPLAYMANAGER_H
#define GRAPHICSSCENEDISPLAYMANAGER_H

#include <QObject>
#include <QElapsedTimer>

#include "viriditysessionmanager.h"
#include "graphicsscenedisplay.h"
#include "handlers/patchrequesthandler.h"

class MainThreadGateway;

/*!
 * \defgroup gsd Viridity Graphics Scene Display
 * This module supports publishing one or more graphics scene, for instance the root item of a Qt Quick scene or a QGraphicsScene instance, to a remote client.
 * It streams graphical changes in a graphics scene to the remote client and also takes care of user interaction within the current session.
 * \sa ViriditySession, AbstractGraphicsSceneDisplayManager, SingleGraphicsSceneDisplayManager, AbstractMultiGraphicsSceneDisplayManager
 */

/*!
    \addtogroup gsd
    @{
*/

/*!
 * The AbstractGraphicsSceneDisplayManager class provides management and setup of GraphicsSceneDisplay instances.
 * This class can be used as base class for a custom graphics scene manager class.
 *
 * Make sure to implement AbstractGraphicsSceneDisplayManager::createDisplayInstance().
 */

class AbstractGraphicsSceneDisplayManager : public QObject, public ViridityMessageHandler
{
    Q_OBJECT
public:
    AbstractGraphicsSceneDisplayManager(ViriditySession *session, QObject *parent = 0);
    virtual ~AbstractGraphicsSceneDisplayManager();

    GraphicsSceneDisplay *getNewDisplay(const QString &id, const QStringList &params);

    GraphicsSceneDisplay *getDisplay(const QString &id);

    GraphicsSceneDisplay *acquireDisplay(const QString &id);
    void releaseDisplay(GraphicsSceneDisplay *display);

    int displayCount() const { return displays_.count(); }

    static QList<AbstractGraphicsSceneDisplayManager *> activeDisplayManagers();

    ViriditySession *session() const { return session_; }

signals:
    void newDisplayCreated(GraphicsSceneDisplay *display);
    void displayRemoved(GraphicsSceneDisplay *display);

protected:
    void removeDisplay(GraphicsSceneDisplay *display);

    // ViridityMessageHandler
    virtual bool canHandleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);
    virtual bool handleMessage(const QByteArray &message, const QString &sessionId, const QString &targetId);

protected slots:
    friend class MainThreadGateway;
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params) = 0; // Always executed in thread of qApp
    virtual void tearDownDisplayInstance(GraphicsSceneDisplay *display); // Always executed in thread of qApp

private slots:
    void killObsoleteDisplays();
    void killAllDisplays();

    void handleDisplayUpdateAvailable();
    void handleSessionDestroyed();

private:
    ViriditySession *session_;
    QMutex displayMutex_;
    QHash<QString, GraphicsSceneDisplay *> displays_;

    struct DisplayResource {
        GraphicsSceneDisplay *display;
        QElapsedTimer lastUsed;
        int useCount;
    };

    QHash<GraphicsSceneDisplay *, DisplayResource> displayResources_;
    QTimer *cleanupTimer_;

    static QReadWriteLock activeDisplayManagersMREW_;
    static QList<AbstractGraphicsSceneDisplayManager *> activeDisplayManagers_;

    PatchRequestHandler *patchRequestHandler_;

    MainThreadGateway *mainThreadGateway_;
};

/*!
 * The SingleGraphicsSceneDisplayManager class provides the simplest form of a graphics scene display manager supporting only one graphics scene and command interpreter shared by all displays.
 * The graphics scene is encapsulated by a AbstractGraphicsSceneAdapter instance which is passed to the constructor of this display manager.
 * SingleGraphicsSceneDisplayManager does not support on-demand instantiation of graphics scenes. As a matter of fact IDs and parameters passed by the client are ignored.
 */

class SingleGraphicsSceneDisplayManager : public AbstractGraphicsSceneDisplayManager
{
    Q_OBJECT
public:
    SingleGraphicsSceneDisplayManager(ViriditySession *session, QObject *parent, AbstractGraphicsSceneAdapter *adapter);
    virtual ~SingleGraphicsSceneDisplayManager();

    AbstractGraphicsSceneAdapter *adapter() const { return adapter_; }
    GraphicsSceneDisplayCommandInterpreter *commandInterpreter() const { return commandInterpreter_; }

    EncoderSettings &encoderSettings() { return es_; }
    ComparerSettings &comparerSettings() { return cs_; }

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params);

private:
    AbstractGraphicsSceneAdapter *adapter_;
    GraphicsSceneDisplayCommandInterpreter *commandInterpreter_;
    EncoderSettings es_;
    ComparerSettings cs_;
};


/*!
 * The AbstractMultiGraphicsSceneDisplayManager class
 */

class AbstractMultiGraphicsSceneDisplayManager : public AbstractGraphicsSceneDisplayManager
{
    Q_OBJECT
public:
    AbstractMultiGraphicsSceneDisplayManager(ViriditySession *session, QObject *parent = 0);
    virtual ~AbstractMultiGraphicsSceneDisplayManager();

protected slots:
    virtual GraphicsSceneDisplay *createDisplayInstance(const QString &id, const QStringList &params);
    virtual void tearDownDisplayInstance(GraphicsSceneDisplay *display);

    virtual AbstractGraphicsSceneAdapter *getAdapter(const QString &id, const QStringList &params) = 0;
    virtual void tearDownAdapter(const QString &id, AbstractGraphicsSceneAdapter *adapter);
};

/*! @}*/

#endif // GRAPHICSSCENEDISPLAYMANAGER_H
