
TARGET = maly
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbMALY.h \
  dbMALYReader.h \
  dbMALYFormat.h \

SOURCES = \
  dbMALY.cc \
  dbMALYReader.cc \
  gsiDeclDbMALY.cc \
