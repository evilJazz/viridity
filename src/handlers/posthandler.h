/****************************************************************************
**
** Copyright (C) 2012-2019 Andre Beckedorf, Meteora Softworks
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

#ifndef POSTHANDLER_H
#define POSTHANDLER_H

#include <QObject>

#include "viridityrequesthandler.h"

class PostHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QByteArray data READ data NOTIFY dataReceived)
    Q_PROPERTY(bool isFinished READ isFinished NOTIFY dataFinished)
public:
    explicit PostHandler(QSharedPointer<ViridityHttpServerRequest> request, QSharedPointer<ViridityHttpServerResponse> response, QObject *parent = 0);
    virtual ~PostHandler();

    QByteArray data() const { return data_; }
    bool isFinished() const { return finished_; }

signals:
    void dataReceived(const QByteArray &chunk);
    void dataFinished();

protected slots:
    virtual void handleRequestData(const QByteArray &chunk);
    virtual void handleRequestEnd();
    virtual void handleRequestClose();

protected:
    QSharedPointer<ViridityHttpServerRequest> request_;
    QSharedPointer<ViridityHttpServerResponse> response_;
    QByteArray data_;
    bool finished_;
};

#endif // POSTHANDLER_H
