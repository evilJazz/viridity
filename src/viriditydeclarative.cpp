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

#include "viriditydeclarative.h"

#include "viriditydatabridge.h"

#ifdef USE_QTQUICK2
    #include <QtQml>
#else
    #include <QtDeclarative>
#endif

#ifdef VIRIDITY_MODULE_DISPLAY
    #include "display/viridityqtquickdisplay.h"
#endif

void ViridityDeclarative::registerTypes()
{
    qmlRegisterType<ViridityNativeDataBridge>("Viridity", 1, 0, "ViridityNativeDataBridge");
    qmlRegisterUncreatableType<ViriditySession>("Viridity", 1, 0, "ViriditySession", "Can't create a session out of thin air.");

#ifdef VIRIDITY_MODULE_DISPLAY
    qmlRegisterType<ViridityQtQuickDisplay>("Viridity", 1, 0, "ViridityDisplay");
#endif
}
