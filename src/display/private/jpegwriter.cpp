#include "jpegwriter.h"

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qimage.h>
#include <qvariant.h>
#include <qvector.h>
#include <qbuffer.h>
#include <qmath.h>

#include <stdio.h>      // jpeglib needs this to be pre-included
#include <setjmp.h>

#ifdef FAR
#undef FAR
#endif

// including jpeglib.h seems to be a little messy
extern "C" {
// mingw includes rpcndr.h but does not define boolean
#if defined(Q_OS_WIN) && defined(Q_CC_GNU)
#   if defined(__RPCNDR_H__) && !defined(boolean)
        typedef unsigned char boolean;
#       define HAVE_BOOLEAN
#   endif
#endif

#define XMD_H           // shut JPEGlib up
#if defined(Q_OS_UNIXWARE)
#  define HAVE_BOOLEAN  // libjpeg under Unixware seems to need this
#endif
#include <jpeglib.h>
#ifdef const
#  undef const          // remove crazy C hackery in jconfig.h
#endif
}

struct my_error_mgr : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};

static void my_error_exit (j_common_ptr cinfo)
{
    my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    qWarning("%s", buffer);
    longjmp(myerr->setjmp_buffer, 1);
}

static void my_output_message(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    qWarning("%s", buffer);
}

static const int max_buf = 4096;

struct my_jpeg_destination_mgr : public jpeg_destination_mgr {
    // Nothing dynamic - cannot rely on destruction over longjump
    QIODevice *device;
    JOCTET buffer[max_buf];

public:
    my_jpeg_destination_mgr(QIODevice *);
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static void qt_init_destination(j_compress_ptr)
{
}

static boolean qt_empty_output_buffer(j_compress_ptr cinfo)
{
    my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;

    int written = dest->device->write((char*)dest->buffer, max_buf);
    if (written == -1)
        (*cinfo->err->error_exit)((j_common_ptr)cinfo);

    dest->next_output_byte = dest->buffer;
    dest->free_in_buffer = max_buf;

#if defined(Q_OS_UNIXWARE)
    return B_TRUE;
#else
    return true;
#endif
}

static void qt_term_destination(j_compress_ptr cinfo)
{
    my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;
    qint64 n = max_buf - dest->free_in_buffer;

    qint64 written = dest->device->write((char*)dest->buffer, n);
    if (written == -1)
        (*cinfo->err->error_exit)((j_common_ptr)cinfo);
}

#if defined(Q_C_CALLBACKS)
}
#endif

inline my_jpeg_destination_mgr::my_jpeg_destination_mgr(QIODevice *device)
{
    jpeg_destination_mgr::init_destination = qt_init_destination;
    jpeg_destination_mgr::empty_output_buffer = qt_empty_output_buffer;
    jpeg_destination_mgr::term_destination = qt_term_destination;
    this->device = device;
    next_output_byte = buffer;
    free_in_buffer = max_buf;
}

inline void set_text(const QImage &image, j_compress_ptr cinfo, const QString &description)
{
    QMap<QString, QString> text;
    foreach (const QString &key, image.textKeys()) {
        if (!key.isEmpty())
            text.insert(key, image.text(key));
    }
    foreach (const QString &pair, description.split(QLatin1String("\n\n"))) {
        int index = pair.indexOf(QLatin1Char(':'));
        if (index >= 0 && pair.indexOf(QLatin1Char(' ')) < index) {
            QString s = pair.simplified();
            if (!s.isEmpty())
                text.insert(QLatin1String("Description"), s);
        } else {
            QString key = pair.left(index);
            if (!key.simplified().isEmpty())
                text.insert(key, pair.mid(index + 2).simplified());
        }
    }
    if (text.isEmpty())
        return;

    for (QMap<QString, QString>::ConstIterator it = text.constBegin(); it != text.constEnd(); ++it) {
        QByteArray comment = it.key().toLatin1();
        if (!comment.isEmpty())
            comment += ": ";
        comment += it.value().toLatin1();
        if (comment.length() > 65530)
            comment.truncate(65530);
        jpeg_write_marker(cinfo, JPEG_COM, (JOCTET *)comment.constData(), comment.size());
    }
}

bool writeJPEG(const QImage &image, QIODevice *device, volatile int sourceQuality, bool optimize, bool progressive, const QString &description)
{
    bool success = false;
    const QVector<QRgb> cmap = image.colorTable();

    struct jpeg_compress_struct cinfo;
    JSAMPROW row_pointer[1];
    row_pointer[0] = 0;

    struct my_jpeg_destination_mgr *iod_dest = new my_jpeg_destination_mgr(device);
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = my_error_exit;
    jerr.output_message = my_output_message;

    if (!setjmp(jerr.setjmp_buffer)) {
        // WARNING:
        // this if loop is inside a setjmp/longjmp branch
        // do not create C++ temporaries here because the destructor may never be called
        // if you allocate memory, make sure that you can free it (row_pointer[0])
        jpeg_create_compress(&cinfo);

        cinfo.dest = iod_dest;

        cinfo.image_width = image.width();
        cinfo.image_height = image.height();

        bool gray=false;
        switch (image.format()) {
        case QImage::Format_Mono:
        case QImage::Format_MonoLSB:
        case QImage::Format_Indexed8:
            gray = true;
            for (int i = image.colorCount(); gray && i--;) {
                gray = gray & (qRed(cmap[i]) == qGreen(cmap[i]) &&
                               qRed(cmap[i]) == qBlue(cmap[i]));
            }
            cinfo.input_components = gray ? 1 : 3;
            cinfo.in_color_space = gray ? JCS_GRAYSCALE : JCS_RGB;
            break;
        default:
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_RGB;
        }

        jpeg_set_defaults(&cinfo);

        qreal diffInch = qAbs(image.dotsPerMeterX()*2.54/100. - qRound(image.dotsPerMeterX()*2.54/100.))
                         + qAbs(image.dotsPerMeterY()*2.54/100. - qRound(image.dotsPerMeterY()*2.54/100.));
        qreal diffCm = (qAbs(image.dotsPerMeterX()/100. - qRound(image.dotsPerMeterX()/100.))
                        + qAbs(image.dotsPerMeterY()/100. - qRound(image.dotsPerMeterY()/100.)))*2.54;
        if (diffInch < diffCm) {
            cinfo.density_unit = 1; // dots/inch
            cinfo.X_density = qRound(image.dotsPerMeterX()*2.54/100.);
            cinfo.Y_density = qRound(image.dotsPerMeterY()*2.54/100.);
        } else {
            cinfo.density_unit = 2; // dots/cm
            cinfo.X_density = (image.dotsPerMeterX()+50) / 100;
            cinfo.Y_density = (image.dotsPerMeterY()+50) / 100;
        }

        if (optimize)
            cinfo.optimize_coding = true;

        if (progressive)
            jpeg_simple_progression(&cinfo);

        int quality = sourceQuality >= 0 ? qMin(int(sourceQuality),100) : 75;
#if defined(Q_OS_UNIXWARE)
        jpeg_set_quality(&cinfo, quality, B_TRUE /* limit to baseline-JPEG values */);
        jpeg_start_compress(&cinfo, B_TRUE);
#else
        jpeg_set_quality(&cinfo, quality, true /* limit to baseline-JPEG values */);
        jpeg_start_compress(&cinfo, true);
#endif

        set_text(image, &cinfo, description);

        row_pointer[0] = new uchar[cinfo.image_width*cinfo.input_components];
        int w = cinfo.image_width;
        while (cinfo.next_scanline < cinfo.image_height) {
            uchar *row = row_pointer[0];
            switch (image.format()) {
            case QImage::Format_Mono:
            case QImage::Format_MonoLSB:
                if (gray) {
                    const uchar* data = image.constScanLine(cinfo.next_scanline);
                    if (image.format() == QImage::Format_MonoLSB) {
                        for (int i=0; i<w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
                            row[i] = qRed(cmap[bit]);
                        }
                    } else {
                        for (int i=0; i<w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (7 -(i & 7))));
                            row[i] = qRed(cmap[bit]);
                        }
                    }
                } else {
                    const uchar* data = image.constScanLine(cinfo.next_scanline);
                    if (image.format() == QImage::Format_MonoLSB) {
                        for (int i=0; i<w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
                            *row++ = qRed(cmap[bit]);
                            *row++ = qGreen(cmap[bit]);
                            *row++ = qBlue(cmap[bit]);
                        }
                    } else {
                        for (int i=0; i<w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (7 -(i & 7))));
                            *row++ = qRed(cmap[bit]);
                            *row++ = qGreen(cmap[bit]);
                            *row++ = qBlue(cmap[bit]);
                        }
                    }
                }
                break;
            case QImage::Format_Indexed8:
                if (gray) {
                    const uchar* pix = image.constScanLine(cinfo.next_scanline);
                    for (int i=0; i<w; i++) {
                        *row = qRed(cmap[*pix]);
                        ++row; ++pix;
                    }
                } else {
                    const uchar* pix = image.constScanLine(cinfo.next_scanline);
                    for (int i=0; i<w; i++) {
                        *row++ = qRed(cmap[*pix]);
                        *row++ = qGreen(cmap[*pix]);
                        *row++ = qBlue(cmap[*pix]);
                        ++pix;
                    }
                }
                break;
            case QImage::Format_RGB888:
                memcpy(row, image.constScanLine(cinfo.next_scanline), w * 3);
                break;
            case QImage::Format_RGB32:
            case QImage::Format_ARGB32:
            case QImage::Format_ARGB32_Premultiplied:
                {
                    const QRgb* rgb = (const QRgb*)image.constScanLine(cinfo.next_scanline);
                    for (int i=0; i<w; i++) {
                        *row++ = qRed(*rgb);
                        *row++ = qGreen(*rgb);
                        *row++ = qBlue(*rgb);
                        ++rgb;
                    }
                }
                break;
            default:
                {
                    // (Testing shows that this way is actually faster than converting to RGB888 + memcpy)
                    QImage rowImg = image.copy(0, cinfo.next_scanline, w, 1).convertToFormat(QImage::Format_RGB32);
                    const QRgb* rgb = (const QRgb*)rowImg.constScanLine(0);
                    for (int i=0; i<w; i++) {
                        *row++ = qRed(*rgb);
                        *row++ = qGreen(*rgb);
                        *row++ = qBlue(*rgb);
                        ++rgb;
                    }
                }
                break;
            }
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        success = true;
    } else {
        jpeg_destroy_compress(&cinfo);
        success = false;
    }

    delete iod_dest;
    delete [] row_pointer[0];
    return success;
}
