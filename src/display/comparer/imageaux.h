#ifndef IMAGEAUX_H
#define IMAGEAUX_H

#include <QColor>
#include <QImage>

class ImageAux
{
public:
    static qreal estimatePNGCompression(const QImage &image, int *estimatedSize = NULL, int compressionLevel = 1);
    static int zlibCompressionLevelToQPNGHandlerQuality(int compressionLevel);

    static QImage createPackedAlphaPatch(const QImage &image);

    static QByteArray removeAncillaryChunksFromPNGStream(const QByteArray &input);

    static QColor getSolidRectColor(QImage *buffer, const QRect &area);
    static QRect findChangedRect32(QImage *buffer1, QImage *buffer2, const QRect &searchArea);

    template<typename T> inline static bool contentMatches(const T *pBuf1, const T *pBuf2, const int stride, const int bytes, const int height)
    {
        for (int y = 0; y < height; ++y)
        {
            if (*pBuf1 != *pBuf2 ||
                memcmp(pBuf1, pBuf2, bytes) != 0)
                return false;

            pBuf1 += stride;
            pBuf2 += stride;
        }

        return true;
    }

    template<typename T> inline static bool contentMatches(QImage *buffer1, QImage *buffer2, const int x1, const int y1, const int x2, const int y2, const int width, const int height)
    {
        const int bytes = width * sizeof(T);
        register const T *pBuf1, *pBuf2;

        pBuf1 = (T *)buffer1->constScanLine(y1) + x1;
        pBuf2 = (T *)buffer2->constScanLine(y2) + x2;

        register const int stride = buffer1->bytesPerLine() / sizeof(T);

        return ImageAux::contentMatches<T>(pBuf1, pBuf2, stride, bytes, height);
    }

    template<typename T> inline static bool contentMatches(QImage *buffer1, QImage *buffer2, const QPoint &pointBuffer1, const QRect &rectBuffer2)
    {
        QRect rect1 = QRect(pointBuffer1, rectBuffer2.size()).intersected(buffer1->rect());
        QRect rect2 = rectBuffer2.intersected(buffer2->rect());

        if (rect1.width() != rect2.width() ||
            rect1.height() != rect2.height())
            return false;

        return ImageAux::contentMatches<T>(buffer1, buffer2, rect1.left(), rect1.top(), rect2.left(), rect2.top(), rect1.width(), rect1.height());
    }
};

#endif // IMAGEAUX_H
