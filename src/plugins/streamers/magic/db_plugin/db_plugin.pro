
TARGET = mag
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbMAG.h \
  dbMAGReader.h \
  dbMAGWriter.h \
  dbMAGFormat.h \

SOURCES = \
  dbMAG.cc \
  dbMAGReader.cc \
  dbMAGWriter.cc \
  gsiDeclDbMAG.cc \
