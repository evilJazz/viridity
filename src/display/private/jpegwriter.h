#ifndef JPEGHANDLER_H
#define JPEGHANDLER_H

#include <QImage>
#include <QIODevice>

bool writeJPEG(const QImage &image, QIODevice *device, volatile int sourceQuality, bool optimize = true, bool progressive = true, const QString &description = QString::null);

#endif // JPEGHANDLER_H
