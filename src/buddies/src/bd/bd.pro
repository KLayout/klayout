
DESTDIR = $$OUT_PWD/../../..
TARGET = klayout_bd

include($$PWD/../../../lib.pri)

DEFINES += MAKE_BD_LIBRARY

SOURCES = \
  bdInit.cc \
  bdReaderOptions.cc \
  bdWriterOptions.cc \
  bdConverterMain.cc \
  strm2cif.cc \
  strm2gds.cc \
  strm2oas.cc \
  strmclip.cc \
  strm2dxf.cc \
  strm2gdstxt.cc \
  strm2txt.cc \
  strmcmp.cc \
  strmxor.cc \
  strmrun.cc \

HEADERS = \
  bdCommon.h \
  bdInit.h \
  bdReaderOptions.h \
  bdWriterOptions.h \
  bdConverterMain.h \

RESOURCES = \

INCLUDEPATH += $$TL_INC $$GSI_INC $$VERSION_INC $$DB_INC $$RBA_INC $$PYA_INC $$LIB_INC $$RDB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$VERSION_INC $$DB_INC $$RBA_INC $$PYA_INC $$LIB_INC $$RDB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi -lklayout_rba -lklayout_pya -lklayout_lib -lklayout_rdb

