
TARGET = net_tracer_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$PWD/../db_plugin
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -lnet_tracer

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
  layNetTracerConfig.h \
  layNetTracerDialog.h \
  layNetTracerIO.h \

SOURCES = \
  layNetTracerConfig.cc \
  layNetTracerDialog.cc \
  layNetTracerPlugin.cc \
  layNetTracerIO.cc \

FORMS = \
  NetTracerConfigPage.ui \
  NetTracerDialog.ui \
  NetTracerTechComponentEditor.ui \

