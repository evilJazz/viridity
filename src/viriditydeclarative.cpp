#include "viriditydeclarative.h"

#include "viriditydatabridge.h"

#ifdef USE_QTQUICK2
    #include <QtQml>
#else
    #include <QtDeclarative>
#endif

void ViridityDeclarative::registerTypes()
{
    qmlRegisterType<ViridityNativeDataBridge>("Viridity", 1, 0, "ViridityNativeDataBridge");
    qmlRegisterUncreatableType<ViriditySession>("Viridity", 1, 0, "ViriditySession", "Can't create a session out of thin air.");
}
