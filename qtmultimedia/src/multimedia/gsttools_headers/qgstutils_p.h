/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGSTUTILS_P_H
#define QGSTUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtCore/qvector.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <qaudioformat.h>
#include <qcamera.h>
#include <qabstractvideobuffer.h>
#include <qvideoframe.h>
#include <QDebug>

#if GST_CHECK_VERSION(1,0,0)
# define QT_GSTREAMER_PLAYBIN_ELEMENT_NAME "playbin"
# define QT_GSTREAMER_CAMERABIN_ELEMENT_NAME "camerabin"
# define QT_GSTREAMER_COLORCONVERSION_ELEMENT_NAME "videoconvert"
# define QT_GSTREAMER_RAW_AUDIO_MIME "audio/x-raw"
# define QT_GSTREAMER_VIDEOOVERLAY_INTERFACE_NAME "GstVideoOverlay"
#else
# define QT_GSTREAMER_PLAYBIN_ELEMENT_NAME "playbin2"
# define QT_GSTREAMER_CAMERABIN_ELEMENT_NAME "camerabin2"
# define QT_GSTREAMER_COLORCONVERSION_ELEMENT_NAME "ffmpegcolorspace"
# define QT_GSTREAMER_RAW_AUDIO_MIME "audio/x-raw-int"
# define QT_GSTREAMER_VIDEOOVERLAY_INTERFACE_NAME "GstXOverlay"
#endif

# ifndef WIN_GST_API
#  ifdef WIN_GST_EXPORT
#    define WIN_GST_API __declspec(dllexport)
#  else
#    define WIN_GST_API __declspec(dllimport)
#  endif
# endif

QT_BEGIN_NAMESPACE

class QSize;
class QVariant;
class QByteArray;
class QImage;
class QVideoSurfaceFormat;

namespace QGstUtils {
    struct WIN_GST_API CameraInfo
    {
        QString name;
        QString description;
        int orientation;
        QCamera::Position position;
        QByteArray driver;
    };

    WIN_GST_API QMap<QByteArray, QVariant> gstTagListToMap(const GstTagList *list);

    WIN_GST_API QSize capsResolution(const GstCaps *caps);
    WIN_GST_API QSize capsCorrectedResolution(const GstCaps *caps);
    WIN_GST_API QAudioFormat audioFormatForCaps(const GstCaps *caps);
#if GST_CHECK_VERSION(1,0,0)
    WIN_GST_API QAudioFormat audioFormatForSample(GstSample *sample);
#else
    WIN_GST_API QAudioFormat audioFormatForBuffer(GstBuffer *buffer);
#endif
    WIN_GST_API GstCaps *capsForAudioFormat(const QAudioFormat &format);
    WIN_GST_API void initializeGst();
    WIN_GST_API QMultimedia::SupportEstimate hasSupport(const QString &mimeType,
                                             const QStringList &codecs,
                                             const QSet<QString> &supportedMimeTypeSet);

    WIN_GST_API QVector<CameraInfo> enumerateCameras(GstElementFactory *factory = 0);
    WIN_GST_API QList<QByteArray> cameraDevices(GstElementFactory * factory = 0);
    WIN_GST_API QString cameraDescription(const QString &device, GstElementFactory * factory = 0);
    WIN_GST_API QCamera::Position cameraPosition(const QString &device, GstElementFactory * factory = 0);
    WIN_GST_API int cameraOrientation(const QString &device, GstElementFactory * factory = 0);
    WIN_GST_API QByteArray cameraDriver(const QString &device, GstElementFactory * factory = 0);

    WIN_GST_API QSet<QString> supportedMimeTypes(bool (*isValidFactory)(GstElementFactory *factory));

#if GST_CHECK_VERSION(1,0,0)
    WIN_GST_API QImage bufferToImage(GstBuffer *buffer, const GstVideoInfo &info);
    WIN_GST_API QVideoSurfaceFormat formatForCaps(
            GstCaps *caps,
            GstVideoInfo *info = 0,
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle);
#else
    WIN_GST_API QImage bufferToImage(GstBuffer *buffer);
    WIN_GST_API QVideoSurfaceFormat formatForCaps(
            GstCaps *caps,
            int *bytesPerLine = 0,
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle);
#endif

    WIN_GST_API GstCaps *capsForFormats(const QList<QVideoFrame::PixelFormat> &formats);
    WIN_GST_API void setFrameTimeStamps(QVideoFrame *frame, GstBuffer *buffer);

    WIN_GST_API void setMetaData(GstElement *element, const QMap<QByteArray, QVariant> &data);
    WIN_GST_API void setMetaData(GstBin *bin, const QMap<QByteArray, QVariant> &data);

    WIN_GST_API GstCaps *videoFilterCaps();

    WIN_GST_API QSize structureResolution(const GstStructure *s);
    WIN_GST_API QVideoFrame::PixelFormat structurePixelFormat(const GstStructure *s, int *bpp = 0);
    WIN_GST_API QSize structurePixelAspectRatio(const GstStructure *s);
    WIN_GST_API QPair<qreal, qreal> structureFrameRateRange(const GstStructure *s);

    WIN_GST_API QString fileExtensionForMimeType(const QString &mimeType);
}

WIN_GST_API void qt_gst_object_ref_sink(gpointer object);
WIN_GST_API GstCaps *qt_gst_pad_get_current_caps(GstPad *pad);
WIN_GST_API GstCaps *qt_gst_pad_get_caps(GstPad *pad);
WIN_GST_API GstStructure *qt_gst_structure_new_empty(const char *name);
WIN_GST_API gboolean qt_gst_element_query_position(GstElement *element, GstFormat format, gint64 *cur);
WIN_GST_API gboolean qt_gst_element_query_duration(GstElement *element, GstFormat format, gint64 *cur);
WIN_GST_API GstCaps *qt_gst_caps_normalize(GstCaps *caps);
WIN_GST_API const gchar *qt_gst_element_get_factory_name(GstElement *element);
WIN_GST_API gboolean qt_gst_caps_can_intersect(const GstCaps * caps1, const GstCaps * caps2);
WIN_GST_API GList *qt_gst_video_sinks();
WIN_GST_API void qt_gst_util_double_to_fraction(gdouble src, gint *dest_n, gint *dest_d);

WIN_GST_API QDebug operator <<(QDebug debug, GstCaps *caps);

QT_END_NAMESPACE

#endif
