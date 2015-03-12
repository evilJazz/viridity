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
