
TARGET = lefdef
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbDEFImporter.h \
  dbLEFDEFImporter.h \
  dbLEFImporter.h

SOURCES = \
  gsiDeclDbLEFDEF.cc \
  dbLEFDEFPlugin.cc \
  dbDEFImporter.cc \
  dbLEFDEFImporter.cc \
  dbLEFImporter.cc
