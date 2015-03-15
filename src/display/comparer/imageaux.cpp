#include "imageaux.h"

#include <QPainter>
#include <qmath.h>

int ImageAux::zlibCompressionLevelToQPNGHandlerQuality(int compressionLevel)
{
    // quality = (100-quality) * 9 / 91; // map [0,100] -> [9,0] in qpnghandler.cpp::write_png_image
    return qFloor(100 - (qreal)(91 * compressionLevel) / 9);
}

qreal ImageAux::estimatePNGCompression(const QImage &image, int *estimatedSize, int compressionLevel)
{
    qreal decimator = qMax(1., (qreal)qMax(image.byteCount(), 1) / (16 * 1024));
    qreal sqrtdec = qSqrt(decimator);
    QSizeF newSize = image.size() / sqrtdec;

    QImage scaledImage = image.scaled(newSize.toSize(), Qt::IgnoreAspectRatio, Qt::FastTransformation);

    QByteArray compressedRaw = qCompress(scaledImage.constBits(), scaledImage.byteCount(), compressionLevel);

    if (estimatedSize)
        *estimatedSize = compressedRaw.size() * decimator * 0.8;

    qreal ratio = (qreal)compressedRaw.size() / scaledImage.byteCount();

    //qDebug("compressedRaw: %d, raw: %d, ratio: %.2f", compressedRaw.size(), scaledImage.byteCount(), ratio);

    return ratio;
}

QImage ImageAux::createPackedAlphaPatch(const QImage &image)
{
    QImage result(image.width(), image.height() * 2, QImage::Format_RGB888);
    result.fill(0);

    QImage inputWithoutAlpha = image.convertToFormat(QImage::Format_RGB888);
    QImage alpha = image.alphaChannel();

    QPainter p;
    p.begin(&result);
    p.drawImage(0, 0, inputWithoutAlpha);
    p.drawImage(0, image.height(), alpha);
    p.end();

    return result;
}

QByteArray ImageAux::removeAncillaryChunksFromPNGStream(const QByteArray &input)
{
    QByteArray result;

    if (input.at(0) == (char)0x89 && input.mid(1, 3) == "PNG")
    {
        result += input.mid(0, 8); // copy PNG file header
        int index = 8;

        while (index < input.size())
        {
            int dataLength = input.mid(index, 4).toHex().toInt(0, 16);
            int chunkLength = 4 + 4 + dataLength + 4;
            QByteArray chunkType = input.mid(index + 4, 4);

            if (chunkType == "IHDR" || chunkType == "PLTE" || chunkType == "IDAT" || chunkType == "IEND")
                result += input.mid(index, chunkLength);

            index += chunkLength;
        }
    }

    return result;
}

QColor ImageAux::getSolidRectColor(QImage *buffer, const QRect &area)
{
    QRect roi = area.intersected(buffer->rect());

    int roiBottom = roi.top() + roi.height();
    int roiRight = roi.left() + roi.width();

    QRgb solidColor = buffer->pixel(roi.topLeft());
    bool isSolidColor = true;
    int y = roi.top();

    QRgb *pBuf;

    while (isSolidColor && y < roiBottom)
    {
        pBuf = (QRgb *)buffer->constScanLine(y) + roi.left();
        ++y;

        for (int x = roi.left(); x < roiRight; ++x)
        {
            if (*pBuf != solidColor)
            {
                isSolidColor = false;
                break;
            }

            ++pBuf;
        }
    }

    if (isSolidColor)
        return QColor::fromRgba(solidColor);
    else
        return QColor();
}

QRect ImageAux::findChangedRect32(QImage *buffer1, QImage *buffer2, const QRect &searchArea)
{
    QRect roi = searchArea.intersected(buffer1->rect()).intersected(buffer2->rect());

    QRect result(roi.bottomRight(), roi.topLeft());

    QRgb *pBuf1, *pBuf2;

    int roiBottom = roi.top() + roi.height();
    int roiRight = roi.left() + roi.width();

    for (int y = roi.top(); y < roiBottom; ++y)
    {
        pBuf1 = (QRgb *)buffer1->constScanLine(y) + roi.left();
        pBuf2 = (QRgb *)buffer2->constScanLine(y) + roi.left();

        for (int x = roi.left(); x < roiRight; ++x)
        {
            if (*pBuf1 != *pBuf2)
            {
                result.setLeft(qMin(result.left(), x));
                result.setTop(qMin(result.top(), y));
                result.setRight(qMax(result.right(), x));
                result.setBottom(qMax(result.bottom(), y));
            }

            ++pBuf1;
            ++pBuf2;
        }
    }

    if (!result.isValid())
        return QRect();
    else
        return result;
}
