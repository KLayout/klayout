
TARGET = net_tracer
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbNetTracer.h \
  dbNetTracerIO.h \

SOURCES = \
  dbNetTracer.cc \
  dbNetTracerIO.cc \
  dbNetTracerPlugin.cc \
  gsiDeclDbNetTracer.cc \

