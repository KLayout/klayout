
TARGET = oasis
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbOASIS.h \
  dbOASISFormat.h \
  dbOASISReader.h \
  dbOASISWriter.h \

SOURCES = \
  dbOASIS.cc \
  dbOASISReader.cc \
  dbOASISWriter.cc \
  gsiDeclDbOASIS.cc \

