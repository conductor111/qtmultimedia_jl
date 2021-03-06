/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "stdafx.h"

#include "player.h"

#include "playercontrols.h"
#include "playlistmodel.h"
#include "histogramwidget.h"
#include "videosettings.h"

//#include <QMediaService>
#include <QMediaPlaylist>
#include <QVideoProbe>
#include <QAudioProbe>
#include <QMediaMetaData>
#include <QtWidgets>
#include <QAudioDecoder>

Player::Player(QWidget *parent)
    : QWidget(parent)
    , videoWidget(0)
    , coverLabel(0)
    , slider(0)
    , colorDialog(0)
{
//! [create-objs]
    player = new QMediaPlayer(this);
    // owned by PlaylistModel
    playlist = new QMediaPlaylist();
    player->setPlaylist(playlist);
//! [create-objs]

    connect(player, SIGNAL(durationChanged(qint64)), SLOT(durationChanged(qint64)));
    connect(player, SIGNAL(positionChanged(qint64)), SLOT(positionChanged(qint64)));
    connect(player, SIGNAL(metaDataChanged()), SLOT(metaDataChanged()));
    connect(playlist, SIGNAL(currentIndexChanged(int)), SLOT(playlistPositionChanged(int)));
    connect(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this, SLOT(statusChanged(QMediaPlayer::MediaStatus)));
    connect(player, SIGNAL(bufferStatusChanged(int)), this, SLOT(bufferingProgress(int)));
    connect(player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(videoAvailableChanged(bool)));
    connect(player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(displayErrorMessage()));
    connect(player, &QMediaPlayer::stateChanged, this, &Player::stateChanged);

//! [2]
    videoWidget = new VideoWidget(this);
    player->setVideoOutput(videoWidget);

    playlistModel = new PlaylistModel(this);
    playlistModel->setPlaylist(playlist);
//! [2]

    playlistView = new QListView(this);
    playlistView->setModel(playlistModel);
    playlistView->setCurrentIndex(playlistModel->index(playlist->currentIndex(), 0));

    connect(playlistView, SIGNAL(activated(QModelIndex)), this, SLOT(jump(QModelIndex)));

    slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, player->duration() / 1000);

    labelDuration = new QLabel(this);
    connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(seek(int)));

    labelHistogram = new QLabel(this);
    labelHistogram->setText("Histogram:");
    videoHistogram = new HistogramWidget(this);
    audioHistogram = new HistogramWidget(this);
    QHBoxLayout *histogramLayout = new QHBoxLayout;
    histogramLayout->addWidget(labelHistogram);
    histogramLayout->addWidget(videoHistogram, 1);
    histogramLayout->addWidget(audioHistogram, 2);

    videoProbe = new QVideoProbe(this);
    connect(videoProbe, SIGNAL(videoFrameProbed(QVideoFrame)), videoHistogram, SLOT(processFrame(QVideoFrame)));
    videoProbe->setSource(player);

    audioProbe = new QAudioProbe(this);
    connect(audioProbe, SIGNAL(audioBufferProbed(QAudioBuffer)), audioHistogram, SLOT(processBuffer(QAudioBuffer)));
    audioProbe->setSource(player);

    QPushButton *openButton = new QPushButton(tr("Open"), this);

    connect(openButton, SIGNAL(clicked()), this, SLOT(open()));

    controls = new PlayerControls(this);
    controls->setState(player->state());
    controls->setVolume(player->volume());
    controls->setMuted(controls->isMuted());
    controls->setEnabled(false);

    connect(controls, SIGNAL(play()), player, SLOT(play()));
    connect(controls, SIGNAL(pause()), player, SLOT(pause()));
    connect(controls, SIGNAL(stop()), player, SLOT(stop()));
    connect(controls, SIGNAL(next()), playlist, SLOT(next()));
    connect(controls, SIGNAL(previous()), this, SLOT(previousClicked()));
    connect(controls, SIGNAL(changeVolume(int)), player, SLOT(setVolume(int)));
    connect(controls, SIGNAL(changeMuting(bool)), player, SLOT(setMuted(bool)));
    connect(controls, SIGNAL(changeRate(qreal)), player, SLOT(setPlaybackRate(qreal)));

    connect(controls, SIGNAL(stop()), videoWidget, SLOT(update()));

    connect(player, SIGNAL(stateChanged(QMediaPlayer::State)),
            controls, SLOT(setState(QMediaPlayer::State)));
    connect(player, SIGNAL(volumeChanged(int)), controls, SLOT(setVolume(int)));
    connect(player, SIGNAL(mutedChanged(bool)), controls, SLOT(setMuted(bool)));

    connect(controls, &PlayerControls::changeRecordButtonState, this, &Player::changeRecordButtonState);
    connect(this, &Player::recordButtonCheck, controls, &PlayerControls::recordButtonSetChecked);

    fullScreenButton = new QPushButton(tr("FullScreen"), this);
    fullScreenButton->setCheckable(true);

    colorButton = new QPushButton(tr("Color Options..."), this);
    colorButton->setEnabled(false);
    connect(colorButton, SIGNAL(clicked()), this, SLOT(showColorDialog()));

    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(videoWidget, 2);
    displayLayout->addWidget(playlistView);

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setMargin(0);
    controlLayout->addWidget(openButton);
    controlLayout->addStretch(1);
    controlLayout->addWidget(controls);
    controlLayout->addStretch(1);
    controlLayout->addWidget(fullScreenButton);
    controlLayout->addWidget(colorButton);

    QBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(displayLayout);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(slider);
    hLayout->addWidget(labelDuration);
    layout->addLayout(hLayout);
    layout->addLayout(controlLayout);
    layout->addLayout(histogramLayout);

    setLayout(layout);

    if (!isPlayerAvailable()) {
        QMessageBox::warning(this, tr("Service not available"),
                             tr("The QMediaPlayer object does not have a valid service.\n"\
                                "Please check the media service plugins are installed."));

        controls->setEnabled(false);
        playlistView->setEnabled(false);
        openButton->setEnabled(false);
        colorButton->setEnabled(false);
        fullScreenButton->setEnabled(false);
    }

    metaDataChanged();
}

Player::~Player()
{
}

bool Player::isPlayerAvailable() const
{
    return player->isAvailable();
}

void Player::changeRecordButtonState(bool recordButtonIsChecked)
{
    if (!mediaRecorder)
    {
        emit recordButtonCheck(false);
        return;
    }

    if (recordButtonIsChecked)
    {
        VideoSettings settingsDialog(mediaRecorder.get());
        settingsDialog.setWindowFlags(settingsDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);

        settingsDialog.setAudioSettings(audioSettings);
        settingsDialog.setVideoSettings(videoSettings);
        settingsDialog.setFormat(videoContainerFormat);

        auto showMessageErrorDlg = [this]()
        {
            QMessageBox::warning(this, tr("Error"), tr("Canceled by user."), QMessageBox::Ok);
            emit recordButtonCheck(false);
        };

        if (settingsDialog.exec() != QDialog::Accepted)
        {
            showMessageErrorDlg();
            return;
        }
        
        audioSettings = settingsDialog.audioSettings();
        videoSettings = settingsDialog.videoSettings();

        // jl
        // you can use any enconder parameter
        // see gstreamer documentation
        // CHECK TYPES!
        if ("video/x-h264" == videoSettings.codec())
        {
            // GstX264EncTune
            // zerolatency(0x00000004) � Zero latency
            videoSettings.setEncodingOption("tune", 4);
            
            // Not need - just example
            // CHECK TYPES!
            // videoSettings.setEncodingOption("ip-factor", float(1.5));
            // videoSettings.setEncodingOption("bframes", uint32_t(1));
        }
        //////////////////////////////////////////////////////////////////////////

        videoContainerFormat = settingsDialog.format();

        mediaRecorder->setEncodingSettings(
            audioSettings,
            videoSettings,
            videoContainerFormat);

        QString selfilter = tr("All files (*.*)");
        QString filename = QFileDialog::getSaveFileName(this, QString(), QString(), selfilter, &selfilter);
        if (filename.isEmpty())
        {
            showMessageErrorDlg();
            return;
        }

        mediaRecorder->setOutputLocation(QUrl::fromLocalFile(filename));
        mediaRecorder->record();
    }
    else
    {
        mediaRecorder->stop();
    }
}

//////////////////////////////////////////////////////////////////////////
class AppSrcDevice : public QIODevice
{
public:
    AppSrcDevice(QObject* parent = 0);
    virtual ~AppSrcDevice() {}

    virtual bool open(OpenMode mode = ReadOnly) override
    {
        setOpenMode(ReadOnly | Unbuffered);
        pos = pos_init_val;
        return true;
    }

    virtual void close() override {}

    bool isSequential() const { return true; }

    virtual qint64 bytesAvailable() const
    {
        return m_ba.length();
    }

protected:
    virtual qint64 readData(char* data, qint64 maxSize) override;
    qint64 writeData(const char* data, qint64 maxSize) { return 0; }

    const size_t pos_init_val = 0;

    QImage m_img;
    QByteArray m_ba;

    size_t pos = pos_init_val;
    qint64 countReadImage = 0;

    Q_DISABLE_COPY(AppSrcDevice)
};

AppSrcDevice::AppSrcDevice(QObject* parent)
    : QIODevice(parent)
{
    m_img.load(QDir::currentPath() + "\\test_frame1.bmp");
    QImage img = m_img.convertToFormat(QImage::Format_RGB888);
    QImage::Format fmt = img.format();
    m_ba.resize(img.byteCount());
    ::memcpy(m_ba.data(), img.bits(), img.byteCount());
}

qint64 AppSrcDevice::readData(char* data, qint64 maxSize)
{
    if (pos == m_ba.size())
    {
        countReadImage++;
        pos = pos_init_val;
    }

    qint64 count = std::min<qint64>(maxSize, m_ba.size() - pos);
    ::memcpy(data, m_ba.data() + pos, count);
    pos += count;

    // simple filter (test only)
    for (int i = 0; i < count; i++)
    {
        data[i] += countReadImage;
    }
    //////////////////////////////////////////////////////////////////////////

    return count;
}

//////////////////////////////////////////////////////////////////////////
class AppSrcDevice2 : public AppSrcDevice
{
public:
    AppSrcDevice2(QObject* parent = 0);
    virtual ~AppSrcDevice2() {}

protected:
    virtual qint64 readData(char* data, qint64 maxSize) override;

    Q_DISABLE_COPY(AppSrcDevice2)
};

AppSrcDevice2::AppSrcDevice2(QObject* parent) :
    AppSrcDevice(parent)
{
    m_img.load(QDir::currentPath() + "\\test_frame1.bmp");
    QImage img = m_img.convertToFormat(QImage::Format_RGB888);
    QImage::Format fmt = img.format();
    m_ba.resize(img.byteCount());
    ::memcpy(m_ba.data(), img.bits(), img.byteCount());
}

qint64 AppSrcDevice2::readData(char* data, qint64 maxSize)
{
    if (pos == m_ba.size())
    {
        countReadImage++;
        pos = pos_init_val;
    }

    qint64 count = std::min<qint64>(maxSize, m_ba.size() - pos);
    ::memcpy(data, m_ba.data() + pos, count);
    pos += count;

    // simple filter (test only)
    for (int i = 0; i < count; i++)
    {
        data[i] += countReadImage;
    }
    //////////////////////////////////////////////////////////////////////////

    return count;
}
//////////////////////////////////////////////////////////////////////////

void Player::open()
{
    //////////////////////////////////////////////////////////////////////////
    // AppSrcDevice using
    //
    // If this is uncommented, 
    // the logic of the original player will work
//     if (true)
//     {
//         controls->setEnabled(true);
//     }
//     else
    {
        if (!appSrcDevice)
        {
            appSrcDevice.reset(new AppSrcDevice());
            appSrcDevice->open();
        }

        if (!appSrcDeviceRecord)
        {
            appSrcDeviceRecord.reset(new AppSrcDevice2());
            appSrcDeviceRecord->open();
        }

        QNetworkRequest request;
        QStringList params;
        // see gst_caps_from_string
        params.append(QString("caps:") + "video/x-raw,format=RGB,width=768,height=576,framerate=25/1");
        // see g_object_set(gstappsrc, "is-live" ...
        params.append(QString("is-live:") + "true");
        // see g_object_set(gstappsrc, "block" ...
        params.append(QString("block:") + "false");
        // see gst_app_src_set_max_bytes
        params.append(QString("max_bytes:") + QString::number(16 * 1024 * 1024));
        // this QIODevice will be used to query the image data to record it to the output video file
        params.append(QString("appSrcStream:") + QString::number((qulonglong)appSrcDeviceRecord.get()));

        // for h264 decoding default values of video queue are not sufficient (on seek position, on change video rate) - 0 is unlimited
        // see gstreamer queue params: max-size-buffers, max-size-bytes, max-size-time, leaky
        // default values: max-size-buffers = 0,  max-size-bytes = 100 * 1024 * 1024, max-size-time = 0, leaky = 0
        params.append("vqueue_max-size-buffers:" + QString::number(0));
        params.append("vqueue_max-size-bytes:" + QString::number(90 * 1024 * 1024));
        params.append("vqueue_max-size-time:" + QString::number(uint64_t(0)));
        params.append("vqueue_leaky:" + QString::number(0));

        // for h264 decoding default values of audio queue may not be sufficient - 0 is unlimited
        // see gstreamer queue params: max-size-buffers, max-size-bytes, max-size-time, leaky
        // default values: max-size-buffers = 0,  max-size-bytes = 100 * 1024 * 1024, max-size-time = 0, leaky = 0
        params.append("aqueue_max-size-buffers:" + QString::number(0));
        params.append("aqueue_max-size-bytes:" + QString::number(90 * 1024 * 1024));
        params.append("aqueue_max-size-time:" + QString::number(uint64_t(0)));
        params.append("aqueue_leaky:" + QString::number(0));

        // Capture session params
        /////////////////////////
        QString additionalParamsPrefix = "QGCS:";
        
        // we can enable/disable PreviewAndRecordingPipeline in the capture session
        // it allows to prevent (if PreviewAndRecordingPipelineEnable = false) the pipeline from freezing in some cases 
        // (x264 Matroska for example) without changing the video queue parameters
        // without PreviewAndRecordingPipeline = true video and audio streams may be out of sync, default value: true
        bool qgcsPreviewAndRecordingPipelineEnable = true;
        params.append(additionalParamsPrefix + "PreviewAndRecordingPipelineEnable:" + (qgcsPreviewAndRecordingPipelineEnable ? "true" : "false"));

        // Queues params
        // for h264 encoding default values of video queue may not be sufficient (pipeline freezing) - 0 is unlimited
        // see gstreamer queue params: max-size-buffers, max-size-bytes, max-size-time, leaky
        // default values: max-size-buffers = 0,  max-size-bytes = 100 * 1024 * 1024, max-size-time = 0, leaky = 0
        params.append(additionalParamsPrefix + "vqueue_max-size-buffers:" + QString::number(0));
        params.append(additionalParamsPrefix + "vqueue_max-size-bytes:" + QString::number(90 * 1024 * 1024));
        params.append(additionalParamsPrefix + "vqueue_max-size-time:" + QString::number(uint64_t(0)));
        params.append(additionalParamsPrefix + "vqueue_leaky:" + QString::number(0));
        // for h264 encoding default values of audio queue may not be sufficient (pipeline freezing) - 0 is unlimited
        // see gstreamer queue params: max-size-buffers, max-size-bytes, max-size-time, leaky
        // default values: max-size-buffers = 0,  max-size-bytes = 100 * 1024 * 1024, max-size-time = 0, leaky = 0
        params.append(additionalParamsPrefix + "aqueue_max-size-buffers:" + QString::number(0));
        params.append(additionalParamsPrefix + "aqueue_max-size-bytes:" + QString::number(90 * 1024 * 1024));
        params.append(additionalParamsPrefix + "aqueue_max-size-time:" + QString::number(uint64_t(0)));
        params.append(additionalParamsPrefix + "aqueue_leaky:" + QString::number(0));

        request.setAttribute(QNetworkRequest::User, params);

        // this QIODevice will be used to query the image data to play it in the player
        player->setMedia(QMediaContent(request), appSrcDevice.get());

        mediaRecorder.reset(new QMediaRecorder(player));
        mediaRecorder->setMetaData(QMediaMetaData::Title, QVariant(QLatin1String("Test Title")));

        controls->setEnabled(true);

        return;
    }
    // end AppSrcDevice using ////////////////////////////////////////////////////////////////////////

    QFileDialog fileDialog(this);
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setWindowTitle(tr("Open Files"));
    QStringList supportedMimeTypes = player->supportedMimeTypes();
    if (!supportedMimeTypes.isEmpty()) {
        supportedMimeTypes.append("audio/x-m3u"); // MP3 playlists
        fileDialog.setMimeTypeFilters(supportedMimeTypes);
    }
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).value(0, QDir::homePath()));
    if (fileDialog.exec() == QDialog::Accepted)
        addToPlaylist(fileDialog.selectedUrls());
}

static bool isPlaylist(const QUrl &url) // Check for ".m3u" playlists.
{
    if (!url.isLocalFile())
        return false;
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists() && !fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive);
}

void Player::addToPlaylist(const QList<QUrl> urls)
{
    foreach (const QUrl &url, urls) {
        if (isPlaylist(url))
            playlist->load(url);
        else
        {
            // jl
            
            QNetworkRequest request(url);

            QStringList params;

            // Playing queues params
            //////////////////////////////////////////////////////////////////////////
            // for h264 decoding default values of video queue are not sufficient (on seek position, on change video rate) - 0 is unlimited
            // see gstreamer queue params: max-size-buffers, max-size-bytes, max-size-time, leaky
            // default values: max-size-buffers = 0,  max-size-bytes = 100 * 1024 * 1024, max-size-time = 0, leaky = 0
            params.append("vqueue_max-size-buffers:" + QString::number(0));
            params.append("vqueue_max-size-bytes:" + QString::number(90 * 1024 * 1024));
            params.append("vqueue_max-size-time:" + QString::number(uint64_t(0)));
            params.append("vqueue_leaky:" + QString::number(0));

            // for h264 decoding default values of audio queue may not be sufficient - 0 is unlimited
            // see gstreamer queue params: max-size-buffers, max-size-bytes, max-size-time, leaky
            // default values: max-size-buffers = 0,  max-size-bytes = 100 * 1024 * 1024, max-size-time = 0, leaky = 0
            params.append("aqueue_max-size-buffers:" + QString::number(0));
            params.append("aqueue_max-size-bytes:" + QString::number(90 * 1024 * 1024));
            params.append("aqueue_max-size-time:" + QString::number(uint64_t(0)));
            params.append("aqueue_leaky:" + QString::number(0));

            request.setAttribute(QNetworkRequest::User, params);

            playlist->addMedia(request);
            //playlist->addMedia(url);
            //////////////////////////////////////////////////////////////////////////
        }
    }
}

void Player::durationChanged(qint64 duration)
{
    this->duration = duration/1000;
    slider->setMaximum(duration / 1000);
}

void Player::positionChanged(qint64 progress)
{
    if (!slider->isSliderDown()) {
        slider->setValue(progress / 1000);
    }
    updateDurationInfo(progress / 1000);
}

void Player::metaDataChanged()
{
    if (player->isMetaDataAvailable()) {
        setTrackInfo(QString("%1 - %2")
                .arg(player->metaData(QMediaMetaData::AlbumArtist).toString())
                .arg(player->metaData(QMediaMetaData::Title).toString()));

        if (coverLabel) {
            QUrl url = player->metaData(QMediaMetaData::CoverArtUrlLarge).value<QUrl>();

            coverLabel->setPixmap(!url.isEmpty()
                    ? QPixmap(url.toString())
                    : QPixmap());
        }
    }
}

void Player::previousClicked()
{
    // Go to previous track if we are within the first 5 seconds of playback
    // Otherwise, seek to the beginning.
    if(player->position() <= 5000)
        playlist->previous();
    else
        player->setPosition(0);
}

void Player::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        playlist->setCurrentIndex(index.row());
        player->play();
    }
}

void Player::playlistPositionChanged(int currentItem)
{
    clearHistogram();
    playlistView->setCurrentIndex(playlistModel->index(currentItem, 0));
}

void Player::seek(int seconds)
{
    player->setPosition(seconds * 1000);
}

void Player::statusChanged(QMediaPlayer::MediaStatus status)
{
    handleCursor(status);

    // handle status message
    switch (status) {
    case QMediaPlayer::UnknownMediaStatus:
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        setStatusInfo(QString());
        break;
    case QMediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case QMediaPlayer::StalledMedia:
        setStatusInfo(tr("Media Stalled"));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        break;
    case QMediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}

void Player::stateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::StoppedState)
        clearHistogram();
}

void Player::handleCursor(QMediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == QMediaPlayer::LoadingMedia ||
        status == QMediaPlayer::BufferingMedia ||
        status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
#endif
}

void Player::bufferingProgress(int progress)
{
    setStatusInfo(tr("Buffering %4%").arg(progress));
}

void Player::videoAvailableChanged(bool available)
{
    if (!available) {
        disconnect(fullScreenButton, SIGNAL(clicked(bool)),
                    videoWidget, SLOT(setFullScreen(bool)));
        disconnect(videoWidget, SIGNAL(fullScreenChanged(bool)),
                fullScreenButton, SLOT(setChecked(bool)));
        videoWidget->setFullScreen(false);
    } else {
        connect(fullScreenButton, SIGNAL(clicked(bool)),
                videoWidget, SLOT(setFullScreen(bool)));
        connect(videoWidget, SIGNAL(fullScreenChanged(bool)),
                fullScreenButton, SLOT(setChecked(bool)));

        if (fullScreenButton->isChecked())
            videoWidget->setFullScreen(true);
    }
    colorButton->setEnabled(available);
}

void Player::setTrackInfo(const QString &info)
{
    trackInfo = info;
    if (!statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(trackInfo).arg(statusInfo));
    else
        setWindowTitle(trackInfo);
}

void Player::setStatusInfo(const QString &info)
{
    statusInfo = info;
    if (!statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(trackInfo).arg(statusInfo));
    else
        setWindowTitle(trackInfo);
}

void Player::displayErrorMessage()
{
    setStatusInfo(player->errorString());
}

void Player::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || duration) {
        QTime currentTime((currentInfo/3600)%60, (currentInfo/60)%60, currentInfo%60, (currentInfo*1000)%1000);
        QTime totalTime((duration/3600)%60, (duration/60)%60, duration%60, (duration*1000)%1000);
        QString format = "mm:ss";
        if (duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    labelDuration->setText(tStr);
}

void Player::showColorDialog()
{
    if (!colorDialog) {
        QSlider *brightnessSlider = new QSlider(Qt::Horizontal);
        brightnessSlider->setRange(-100, 100);
        brightnessSlider->setValue(videoWidget->brightness());
        connect(brightnessSlider, SIGNAL(sliderMoved(int)), videoWidget, SLOT(setBrightness(int)));
        connect(videoWidget, SIGNAL(brightnessChanged(int)), brightnessSlider, SLOT(setValue(int)));

        QSlider *contrastSlider = new QSlider(Qt::Horizontal);
        contrastSlider->setRange(-100, 100);
        contrastSlider->setValue(videoWidget->contrast());
        connect(contrastSlider, SIGNAL(sliderMoved(int)), videoWidget, SLOT(setContrast(int)));
        connect(videoWidget, SIGNAL(contrastChanged(int)), contrastSlider, SLOT(setValue(int)));

        QSlider *hueSlider = new QSlider(Qt::Horizontal);
        hueSlider->setRange(-100, 100);
        hueSlider->setValue(videoWidget->hue());
        connect(hueSlider, SIGNAL(sliderMoved(int)), videoWidget, SLOT(setHue(int)));
        connect(videoWidget, SIGNAL(hueChanged(int)), hueSlider, SLOT(setValue(int)));

        QSlider *saturationSlider = new QSlider(Qt::Horizontal);
        saturationSlider->setRange(-100, 100);
        saturationSlider->setValue(videoWidget->saturation());
        connect(saturationSlider, SIGNAL(sliderMoved(int)), videoWidget, SLOT(setSaturation(int)));
        connect(videoWidget, SIGNAL(saturationChanged(int)), saturationSlider, SLOT(setValue(int)));

        QFormLayout *layout = new QFormLayout;
        layout->addRow(tr("Brightness"), brightnessSlider);
        layout->addRow(tr("Contrast"), contrastSlider);
        layout->addRow(tr("Hue"), hueSlider);
        layout->addRow(tr("Saturation"), saturationSlider);

        QPushButton *button = new QPushButton(tr("Close"));
        layout->addRow(button);

        colorDialog = new QDialog(this);
        colorDialog->setWindowTitle(tr("Color Options"));
        colorDialog->setLayout(layout);

        connect(button, SIGNAL(clicked()), colorDialog, SLOT(close()));
    }
    colorDialog->show();
}

void Player::clearHistogram()
{
    QMetaObject::invokeMethod(videoHistogram, "processFrame", Qt::QueuedConnection, Q_ARG(QVideoFrame, QVideoFrame()));
    QMetaObject::invokeMethod(audioHistogram, "processBuffer", Qt::QueuedConnection, Q_ARG(QAudioBuffer, QAudioBuffer()));
}
