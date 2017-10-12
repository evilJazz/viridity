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

#include "viriditywebserver.h"

#ifndef VIRIDITY_DEBUG
#undef DEBUG
#endif
#include "KCL/debug.h"

#include <QByteArray>
#include <QBuffer>
#include <QFile>
#include <QEventLoop>

#include "viridityconnection.h"
#include "handlers/filerequesthandler.h"
#include "handlers/sessionroutingrequesthandler.h"

/* ViridityWebServer */

ViridityWebServer::ViridityWebServer(QObject *parent, AbstractViriditySessionManager *sessionManager) :
    QTcpServer(parent),
    sessionManager_(sessionManager),
    connectionMREW_(QReadWriteLock::Recursive),
    clearingConnections_(false),
    incomingConnectionCount_(0)
{
    DGUARDMETHODTIMED;

    qRegisterMetaType< QSharedPointer<ViridityConnection> >();

    fileRequestHandler_ = QSharedPointer<ViridityRequestHandler>(new FileRequestHandler(this));
    registerRequestHandler(fileRequestHandler_);

    if (sessionManager_)
    {
        sessionManager_->setServer(this);

        sessionRoutingRequestHandler_ = QSharedPointer<ViridityRequestHandler>(new SessionRoutingRequestHandler(this));
        registerRequestHandler(sessionRoutingRequestHandler_);
    }
}

ViridityWebServer::~ViridityWebServer()
{
    DGUARDMETHODTIMED;
    QReadLocker l(&connectionMREW_);

    if (sessionManager_)
        sessionManager_->killAllSessions();

    close();

    requestHandlers_.clear();

    foreach (QThread *t, connectionThreads_)
        t->quit();

    foreach (QThread *t, connectionThreads_)
    {
        t->wait();
        delete t;
    }

    connectionThreads_.clear();
}

bool ViridityWebServer::listen(const QHostAddress &address, quint16 port, int threadsNumber)
{
    threadsNumber = qMax(1, threadsNumber);

    QThread *newThread;

    connectionThreads_.reserve(threadsNumber);
    for (int i = 0; i < threadsNumber; ++i)
    {
        newThread = new QThread(this);
        newThread->setObjectName("VConnThread" + QString::number(i));
        newThread->start();
        connectionThreads_.append(newThread);
    }

    return QTcpServer::listen(address, port);
}

bool ViridityWebServer::close()
{
    QTcpServer::close();
    closeAllConnections(2000);
}

AbstractViriditySessionManager *ViridityWebServer::sessionManager()
{
    return sessionManager_;
}

void ViridityWebServer::cleanConnections()
{
    DGUARDMETHODTIMED;
    QWriteLocker l(&connectionMREW_);
    DPRINTF("Connection count before: %d", connections_.count());

    for (int i = connections_.count() - 1; i >= 0; --i)
    {
        if (connections_.at(i).isNull())
            connections_.removeAt(i);
    }

    DPRINTF("Connection count after: %d", connections_.count());
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
void ViridityWebServer::incomingConnection(qintptr handle)
#else
void ViridityWebServer::incomingConnection(int handle)
#endif
{
    DGUARDMETHODTIMED;
    QWriteLocker l(&connectionMREW_);

    cleanConnections();

    ++incomingConnectionCount_;
    int threadIndex = incomingConnectionCount_ % connectionThreads_.count();

    QSharedPointer<ViridityConnection> connection(
        new ViridityConnection(this, handle),
        &ViridityConnection::sharedPointerDeleteLater
    );

    connections_.append(connection.toWeakRef());
    connection->moveToThread(connectionThreads_.at(threadIndex)); // Move connection to thread's event loop

    // Dispatch setupConnection call to thread's event loop
    QMetaObject::invokeMethod(
        connection.data(),
        "setupConnection",
        Q_ARG(QSharedPointer<ViridityConnection>, connection) // send QSharedPointer reference to keep alive!
    );
}

void ViridityWebServer::closeAllConnections(int maxWaitMs)
{
    DGUARDMETHODTIMED;

    clearingConnections_ = true;

    {
        QReadLocker l(&connectionMREW_);
        foreach (QWeakPointer<ViridityConnection> connection, connections_)
        {
            QSharedPointer<ViridityConnection> shCon = connection.toStrongRef();
            if (shCon)
                QMetaObject::invokeMethod(
                    shCon.data(),
                    "close",
                    Q_ARG(QSharedPointer<ViridityConnection>, shCon)  // send QSharedPointer reference to keep alive!
                );
        }
    }

    if (maxWaitMs > 0)
    {
        QElapsedTimer timer;
        QEventLoop lo;
        while (connections_.count() > 0 && timer.elapsed() < maxWaitMs)
        {
            cleanConnections();
            lo.processEvents(QEventLoop::AllEvents, 10);
        }
    }

    clearingConnections_ = false;
}

void ViridityWebServer::registerRequestHandler(QSharedPointer<ViridityRequestHandler> handler, bool prepend)
{
    DGUARDMETHODTIMED;
    QWriteLocker l(&requestHandlersMREW_);
    if (requestHandlers_.indexOf(handler) == -1)
    {
        if (prepend)
            requestHandlers_.prepend(handler);
        else
            requestHandlers_.append(handler);
    }
}

void ViridityWebServer::unregisterRequestHandler(QSharedPointer<ViridityRequestHandler> handler)
{
    DGUARDMETHODTIMED;
    QWriteLocker l(&requestHandlersMREW_);
    requestHandlers_.removeAll(handler);
}

bool ViridityWebServer::handleRequest(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response)
{
    QList< QSharedPointer<ViridityRequestHandler> > localList;
    {
        QReadLocker l(&requestHandlersMREW_);
        localList = requestHandlers_;
        localList.detach(); // Required to raise ref counters
    }

    foreach (const QSharedPointer<ViridityRequestHandler> &handler, localList)
        handler->filterRequestResponse(request, response);

    bool result = false;

    foreach (const QSharedPointer<ViridityRequestHandler> &handler, localList)
        if (handler->doesHandleRequest(request))
        {
            handler->handleRequest(request, response);
            result = true;
        }

    return result;
}

QVariant ViridityWebServer::stats() const
{
    QVariantMap result;

    if (sessionManager_)
        result.insert("sessionManager", sessionManager_->stats());

    QReadLocker l(&connectionMREW_);
    QVariantList connectionArray;

    foreach (const QWeakPointer<ViridityConnection> &connection, connections_)
    {
        if (!connection.isNull())
        {
            QSharedPointer<ViridityConnection> shCon = connection.toStrongRef();
            if (!shCon.isNull())
                connectionArray.append(shCon->stats());
        }
    }

    result.insert("connections", connectionArray);

    return result;
}
