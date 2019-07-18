TEMPLATE = lib

TARGET = qgsttools_p
QPRO_PWD = $$PWD

QT = core-private multimedia-private gui-private
LIBS += -lglib-2.0 $$(GSTREAMER_DIR)lib\libglib-2.0.dll.a $$(GSTREAMER_DIR)lib\gobject-2.0.lib $$(GSTREAMER_DIR)lib\gstvideo-1.0.lib $$(GSTREAMER_DIR)lib\gstaudio-1.0.lib $$(GSTREAMER_DIR)lib\gstpbutils-1.0.lib

!static:DEFINES += QT_MAKEDLL
DEFINES += GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26
DEFINES += WIN_GST_EXPORT

qtConfig(alsa): \
    QMAKE_USE += alsa

QMAKE_USE += gstreamer

qtConfig(resourcepolicy): \
    QMAKE_USE += libresourceqt5

# Header files must go inside source directory of a module
# to be installed by syncqt.
INCLUDEPATH += ../multimedia/gsttools_headers/
INCLUDEPATH += ../plugins/gstreamer/mediaplayer/
VPATH += ../multimedia/gsttools_headers/

PRIVATE_HEADERS += \
    qgstreamerbushelper_p.h \
    qgstreamermessage_p.h \
    qgstutils_p.h \
    qgstvideobuffer_p.h \
    qgstreamerbufferprobe_p.h \
    qgstreamervideorendererinterface_p.h \
    qgstreameraudioinputselector_p.h \
    qgstreamervideorenderer_p.h \
    qgstreamervideoinputdevicecontrol_p.h \
    qgstcodecsinfo_p.h \
    qgstreamervideoprobecontrol_p.h \
    qgstreameraudioprobecontrol_p.h \
    qgstreamervideowindow_p.h \
    qgstreamervideooverlay_p.h

SOURCES += \
    qgstreamerbushelper.cpp \
    qgstreamermessage.cpp \
    qgstutils.cpp \
    qgstvideobuffer.cpp \
    qgstreamerbufferprobe.cpp \
    qgstreamervideorendererinterface.cpp \
    qgstreameraudioinputselector.cpp \
    qgstreamervideorenderer.cpp \
    qgstreamervideoinputdevicecontrol.cpp \
    qgstcodecsinfo.cpp \
    qgstreamervideoprobecontrol.cpp \
    qgstreameraudioprobecontrol.cpp \
    qgstreamervideowindow.cpp \
    qgstreamervideooverlay.cpp

qtHaveModule(widgets) {
    QT += multimediawidgets

    PRIVATE_HEADERS += \
        qgstreamervideowidget_p.h

    SOURCES += \
        qgstreamervideowidget.cpp
}

qtConfig(gstreamer_0_10) {
    PRIVATE_HEADERS += \
        qgstbufferpoolinterface_p.h \
        qvideosurfacegstsink_p.h \
        gstvideoconnector_p.h

    SOURCES += \
        qgstbufferpoolinterface.cpp \
        qvideosurfacegstsink.cpp \
        gstvideoconnector.c
} else {
    PRIVATE_HEADERS += \
        qgstvideorendererplugin_p.h \
        qgstvideorenderersink_p.h

    SOURCES += \
        qgstvideorendererplugin.cpp \
        qgstvideorenderersink.cpp
}

qtConfig(mirclient): {
    qtConfig(opengles2):qtHaveModule(widgets) {
        PRIVATE_HEADERS += qgstreamermirtexturerenderer_p.h
        SOURCES += qgstreamermirtexturerenderer.cpp
        QT += opengl quick
        LIBS += -lEGL
    }
}

qtConfig(gstreamer_app) {
    QMAKE_USE += gstreamer_app
    PRIVATE_HEADERS += qgstappsrc_p.h
    SOURCES += qgstappsrc.cpp
}

HEADERS += $$PRIVATE_HEADERS

DESTDIR = $$QT.multimedia.libs
target.path = $$[QT_INSTALL_LIBS]

INSTALLS += target

load(qt_module)