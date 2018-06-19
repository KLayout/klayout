
TARGET = cif
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbCIF.h \
  dbCIFReader.h \
  dbCIFWriter.h \
  dbCIFFormat.h \

SOURCES = \
  dbCIF.cc \
  dbCIFReader.cc \
  dbCIFWriter.cc \
  gsiDeclDbCIF.cc \
