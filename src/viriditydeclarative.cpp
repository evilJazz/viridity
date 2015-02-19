#include "viriditydeclarative.h"

#include "viriditydatabridge.h"

void ViridityDeclarative::registerTypes()
{
    qmlRegisterType<ViridityDataBridge>("Viridity", 1, 0, "NativeViridityDataBridge");
    qmlRegisterUncreatableType<ViriditySession>("Viridity", 1, 0, "ViriditySession", "Can't create a session out of thin air.");
}
