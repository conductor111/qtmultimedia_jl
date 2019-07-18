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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdebug.h>

#if defined(HAVE_WIDGETS)
#include <QtWidgets/qwidget.h>
#endif

#include "qgstreamerplayerservice.h"
#include "qgstreamerplayercontrol.h"
#include "qgstreamerplayersession.h"
#include "qgstreamermetadataprovider.h"
#include "qgstreameravailabilitycontrol.h"

#if defined(HAVE_WIDGETS)
#include <private/qgstreamervideowidget_p.h>
#endif
#include <private/qgstreamervideowindow_p.h>
#include <private/qgstreamervideorenderer_p.h>

#if QT_CONFIG(mirclient) && defined (__arm__)
#include "private/qgstreamermirtexturerenderer_p.h"
#endif

#include "qgstreamerstreamscontrol.h"
#include <private/qgstreameraudioprobecontrol_p.h>
#include <private/qgstreamervideoprobecontrol_p.h>

#include <private/qmediaplaylistnavigator_p.h>
#include <qmediaplaylist.h>
#include <private/qmediaresourceset_p.h>

// jl
#include "../mediacapture/qgstreamerrecordercontrol.h"
#include "../mediacapture/qgstreameraudioencode.h"
#include "../mediacapture/qgstreamervideoencode.h"
#include "../mediacapture/qgstreamermediacontainercontrol.h"
#ifdef _WIN32
#include <windows.h>
#endif
//////////////////////////////////////////////////////////////////////////

QT_BEGIN_NAMESPACE

// jl
QLibrary QGstreamerPlayerService::s_captureLib;
QFunctionPointer QGstreamerPlayerService::s_captureProc = nullptr;

bool QGstreamerPlayerService::isPlayerCaptureSessionAvailable(bool &existLibrary, bool &existCreator)
{
    existLibrary = existCreator = false;

    if (s_captureProc || s_captureLib.isLoaded())
    {
        existLibrary = true;
        existCreator = s_captureProc != nullptr;

        return true;
    }

#ifdef _DEBUG
    QString libName("gstmediacaptured");
#else
    QString libName("gstmediacapture");
#endif

    QString path;
#ifdef _WIN32
    HMODULE phModule = NULL;
    if (::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)&isPlayerCaptureSessionAvailable, &phModule))
    {
        const int bufLen = 32768;
        wchar_t buf[bufLen];
        if (::GetModuleFileNameW(phModule, buf, sizeof(buf)) > 0)
        {
            wchar_t bufT[bufLen];
            wchar_t* filePart;
            if (::GetFullPathNameW(buf, bufLen, bufT, &filePart) > 0)
            {
                path = QString::fromStdWString(buf);
                int filePartLen = (int)::wcslen(filePart);
                path.remove(path.length() - filePartLen, filePartLen);
            }
        }
    }
#endif
    path += libName;

    s_captureLib.setFileName(path);
    if (!s_captureLib.load())
    {
        return false;
    }

    existLibrary = true;

    s_captureProc = s_captureLib.resolve("createQGstreamerCaptureSessionWrapper");
    if (s_captureProc)
    {
        existCreator = true;
    }

    return s_captureProc != nullptr;
}
//////////////////////////////////////////////////////////////////////////

QGstreamerPlayerService::QGstreamerPlayerService(QObject *parent):
     QMediaService(parent)
     , m_audioProbeControl(0)
     , m_videoProbeControl(0)
     , m_videoOutput(0)
     , m_videoRenderer(0)
     , m_videoWindow(0)
#if defined(HAVE_WIDGETS)
     , m_videoWidget(0)
#endif
     , m_videoReferenceCount(0)
     // jl
     , m_captureSession(0)
    //////////////////////////////////////////////////////////////////////////
{
    m_session = new QGstreamerPlayerSession(this);
    m_control = new QGstreamerPlayerControl(m_session, this);
    m_metaData = new QGstreamerMetaDataProvider(m_session, this);
    m_streamsControl = new QGstreamerStreamsControl(m_session,this);
    m_availabilityControl = new QGStreamerAvailabilityControl(m_control->resources(), this);

#if QT_CONFIG(mirclient) && defined (__arm__)
    m_videoRenderer = new QGstreamerMirTextureRenderer(this, m_session);
#else
    m_videoRenderer = new QGstreamerVideoRenderer(this);
#endif

    m_videoWindow = new QGstreamerVideoWindow(this);
   // If the GStreamer video sink is not available, don't provide the video window control since
    // it won't work anyway.
    if (!m_videoWindow->videoSink()) {
        delete m_videoWindow;
        m_videoWindow = 0;
    }

#if defined(HAVE_WIDGETS)
    m_videoWidget = new QGstreamerVideoWidgetControl(this);

    // If the GStreamer video sink is not available, don't provide the video widget control since
    // it won't work anyway.
    // QVideoWidget will fall back to QVideoRendererControl in that case.
    if (!m_videoWidget->videoSink()) {
        delete m_videoWidget;
        m_videoWidget = 0;
    }
#endif

    // jl
    bool existLibrary = false, existCreator = false;
    if (isPlayerCaptureSessionAvailable(existLibrary, existCreator))
    {
        typedef QGstreamerCaptureSessionWrapper* (*QGCSW)(QGstreamerCaptureSession::CaptureMode, QObject*);
        m_captureSession = ((QGCSW)s_captureProc)(QGstreamerCaptureSession::Video, this);
        m_captureSession->setVideoInput(m_session);
    }
    //////////////////////////////////////////////////////////////////////////
}

QGstreamerPlayerService::~QGstreamerPlayerService()
{
    // jl
    if (m_captureSession)
    {
        m_captureSession->Release();
    }
    //////////////////////////////////////////////////////////////////////////
}

QMediaControl *QGstreamerPlayerService::requestControl(const char *name)
{
    if (qstrcmp(name,QMediaPlayerControl_iid) == 0)
        return m_control;

    if (qstrcmp(name,QMetaDataReaderControl_iid) == 0)
        return m_metaData;

    if (qstrcmp(name,QMediaStreamsControl_iid) == 0)
        return m_streamsControl;

    if (qstrcmp(name, QMediaAvailabilityControl_iid) == 0)
        return m_availabilityControl;

    // jl
    if (qstrcmp(name, QMediaRecorderControl_iid) == 0)
    {
        if (m_captureSession)
        {
            if (m_captureSession->state() == QGstreamerCaptureSession::StoppedState)
            {
                m_captureSession->setState(QGstreamerCaptureSession::PreviewState);
            }
            return m_captureSession->recorderControl();
        }
    }
    
    if (qstrcmp(name, QAudioEncoderSettingsControl_iid) == 0)
    {
        if (m_captureSession)
        {
            return m_captureSession->audioEncodeControl();
        }
    }

    if (qstrcmp(name, QVideoEncoderSettingsControl_iid) == 0)
    {
        if (m_captureSession)
        {
            return m_captureSession->videoEncodeControl();
        }
    }

    if (qstrcmp(name, QMediaContainerControl_iid) == 0)
    {
        if (m_captureSession)
        {
            return m_captureSession->mediaContainerControl();
        }
    }
    //////////////////////////////////////////////////////////////////////////

    if (qstrcmp(name, QMediaVideoProbeControl_iid) == 0) {
        if (!m_videoProbeControl) {
            increaseVideoRef();
            m_videoProbeControl = new QGstreamerVideoProbeControl(this);
            m_session->addProbe(m_videoProbeControl);
        }
        m_videoProbeControl->ref.ref();
        return m_videoProbeControl;
    }

    if (qstrcmp(name, QMediaAudioProbeControl_iid) == 0) {
        if (!m_audioProbeControl) {
            m_audioProbeControl = new QGstreamerAudioProbeControl(this);
            m_session->addProbe(m_audioProbeControl);
        }
        m_audioProbeControl->ref.ref();
        return m_audioProbeControl;
    }

    if (!m_videoOutput) {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            m_videoOutput = m_videoRenderer;
        else if (qstrcmp(name, QVideoWindowControl_iid) == 0)
            m_videoOutput = m_videoWindow;
#if defined(HAVE_WIDGETS)
        else if (qstrcmp(name, QVideoWidgetControl_iid) == 0)
            m_videoOutput = m_videoWidget;
#endif

        if (m_videoOutput) {
            increaseVideoRef();
            m_control->setVideoOutput(m_videoOutput);
            return m_videoOutput;
        }
    }

    return 0;
}

void QGstreamerPlayerService::releaseControl(QMediaControl *control)
{
    if (!control) {
        return;
    } else if (control == m_videoOutput) {
        m_videoOutput = 0;
        m_control->setVideoOutput(0);
        decreaseVideoRef();
    } else if (control == m_videoProbeControl && !m_videoProbeControl->ref.deref()) {
        m_session->removeProbe(m_videoProbeControl);
        delete m_videoProbeControl;
        m_videoProbeControl = 0;
        decreaseVideoRef();
    } else if (control == m_audioProbeControl && !m_audioProbeControl->ref.deref()) {
        m_session->removeProbe(m_audioProbeControl);
        delete m_audioProbeControl;
        m_audioProbeControl = 0;
    }
}

void QGstreamerPlayerService::increaseVideoRef()
{
    m_videoReferenceCount++;
    if (m_videoReferenceCount == 1) {
        m_control->resources()->setVideoEnabled(true);
    }
}

void QGstreamerPlayerService::decreaseVideoRef()
{
    m_videoReferenceCount--;
    if (m_videoReferenceCount == 0) {
        m_control->resources()->setVideoEnabled(false);
    }
}

QT_END_NAMESPACE
