
TARGET = pcb
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  extGerberDrillFileReader.h \
  extGerberImporter.h \
  extRS274XApertures.h \
  extRS274XReader.h \

SOURCES = \
  extGerberDrillFileReader.cc \
  extGerberImporter.cc \
  extRS274XApertures.cc \
  extRS274XReader.cc \
