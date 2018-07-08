
TARGET = pcb
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
    dbGerberDrillFileReader.h \
    dbGerberImporter.h \
    dbRS274XApertures.h \
    dbRS274XReader.h \
    dbGerberImportData.h

SOURCES = \
    dbGerberDrillFileReader.cc \
    dbGerberImporter.cc \
    dbRS274XApertures.cc \
    dbRS274XReader.cc \
    dbGerberImportData.cc
