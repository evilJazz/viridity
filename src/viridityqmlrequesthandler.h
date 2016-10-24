#ifndef VIRIDITYQMLREQUESTHANDLER_H
#define VIRIDITYQMLREQUESTHANDLER_H

#include <QObject>

#include "viriditydeclarative.h"
#include "viridityrequesthandler.h"

class PrivateQmlRequestHandler;

class ViridityQmlRequestHandler : public ViridityDeclarativeBaseObject
{
    Q_OBJECT
public:
    ViridityQmlRequestHandler(QObject *parent = NULL);
    virtual ~ViridityQmlRequestHandler();

    virtual void componentComplete();

private:
    PrivateQmlRequestHandler *requestHandler_;
};

#endif // VIRIDITYQMLREQUESTHANDLER_H
