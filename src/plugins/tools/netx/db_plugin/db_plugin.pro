
TARGET = netx
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbNetExtractor.h \
  dbHierProcessor.h \

SOURCES = \
  dbNetExtractor.cc \
  dbHierProcessor.cc \
  dbNetExtractorPlugin.cc \
  gsiDeclDbNetExtractor.cc \

