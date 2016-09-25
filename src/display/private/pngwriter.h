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

#ifndef PNGWRITER_H
#define PNGWRITER_H

#include <QImage>
#include <QIODevice>

enum PNGFilterFlag {
    PNGNoFilters = 0x00,
    PNGFilterNone = 0x08,
    PNGFilterSub = 0x10,
    PNGFilterUp = 0x20,
    PNGFilterAvg = 0x40,
    PNGFilterPaeth = 0x80,
    PNGAllFilters = PNGFilterNone | PNGFilterSub | PNGFilterUp | PNGFilterAvg | PNGFilterPaeth
};
Q_DECLARE_FLAGS(PNGFilters, PNGFilterFlag)

bool writePNG(const QImage &image, QIODevice *device, int compression, PNGFilters filters, float gamma = 0.0, const QString &description = QString::null);

#endif // PNGWRITER_H
