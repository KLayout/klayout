
TARGET = netx
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbNetExtractor.h \
  dbHierProcessor.h \
    dbLocalOperation.h

SOURCES = \
  dbNetExtractor.cc \
  dbHierProcessor.cc \
  dbNetExtractorPlugin.cc \
  gsiDeclDbNetExtractor.cc \
    dbLocalOperation.cc

