
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

INCLUDEPATH += $$TL_INC $$GSI_INC $$VERSION_INC $$DB_INC $$LIB_INC $$RDB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$VERSION_INC $$DB_INC $$LIB_INC $$RDB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi -lklayout_lib -lklayout_rdb

PLUGINPATH += \
  $$PWD/../../../plugins/common \
  $$PWD/../../../plugins/streamers/gds2/db_plugin \
  $$PWD/../../../plugins/streamers/cif/db_plugin \
  $$PWD/../../../plugins/streamers/oasis/db_plugin \
  $$PWD/../../../plugins/streamers/dxf/db_plugin \

INCLUDEPATH += $$PLUGINPATH
DEPENDPATH += $$PLUGINPATH

LIBS += -L$$DESTDIR/db_plugins \
  -lgds2 \
  -lcif \
  -loasis \
  -ldxf \

INCLUDEPATH += $$RBA_INC
DEPENDPATH += $$RBA_INC

equals(HAVE_RUBY, "1") {
  LIBS += -lklayout_rba
} else {
  LIBS += -lklayout_rbastub
}

INCLUDEPATH += $$PYA_INC
DEPENDPATH += $$PYA_INC

equals(HAVE_PYTHON, "1") {
  LIBS += -lklayout_pya
} else {
  LIBS += -lklayout_pyastub
}


