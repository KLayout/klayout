
TARGET = netx
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbNetExtractor.h \

SOURCES = \
  dbNetExtractor.cc \
  dbNetExtractorPlugin.cc \
  gsiDeclDbNetExtractor.cc \

