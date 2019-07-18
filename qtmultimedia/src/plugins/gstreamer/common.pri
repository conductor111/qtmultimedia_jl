QT += core-private multimedia-private network

qtHaveModule(widgets) {
    QT += widgets multimediawidgets-private
    DEFINES += HAVE_WIDGETS
}

CONFIG(debug, debug|release) {
    LIBS += -lqgsttools_pd
} else {
    LIBS += -lqgsttools_p
}
LIBS += -lglib-2.0 $$(GSTREAMER_DIR)lib\gstapp-1.0.lib $$(GSTREAMER_DIR)lib\gstbase-1.0.lib $$(GSTREAMER_DIR)lib\libglib-2.0.dll.a $$(GSTREAMER_DIR)lib\gobject-2.0.lib $$(GSTREAMER_DIR)lib\gstvideo-1.0.lib $$(GSTREAMER_DIR)lib\gstaudio-1.0.lib $$(GSTREAMER_DIR)lib\gstpbutils-1.0.lib

DEFINES -= QT_BUILD_MULTIMEDIA_LIB

QMAKE_USE += gstreamer

qtConfig(resourcepolicy): \
    QMAKE_USE += libresourceqt5

qtConfig(gstreamer_app): \
    QMAKE_USE += gstreamer_app

