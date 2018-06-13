
TARGET = lefdef
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  extDEFImporter.h \
  extLEFImporter.h \
  extLEFDEFImporter.h \

SOURCES = \
  extDEFImporter.cc \
  extLEFDEFImporter.cc \
  extLEFImporter.cc \
  gsiDeclDbLEFDEF.cc \
