
TARGET = dxf
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  dbDXF.h \
  dbDXFFormat.h \
  dbDXFReader.h \
  dbDXFWriter.h \

SOURCES = \
  dbDXF.cc \
  dbDXFReader.cc \
  dbDXFWriter.cc \
  gsiDeclDbDXF.cc \

