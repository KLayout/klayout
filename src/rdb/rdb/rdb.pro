
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_rdb

include($$PWD/../../lib.pri)

DEFINES += MAKE_RDB_LIBRARY

FORMS = \

RESOURCES = \

SOURCES = \
  gsiDeclRdb.cc \
  rdb.cc \
  rdbForceLink.cc \
  rdbFile.cc \
  rdbReader.cc \
  rdbRVEReader.cc \
  rdbTiledRdbOutputReceiver.cc \
  rdbUtils.cc \

HEADERS = \
  rdb.h \
  rdbForceLink.h \
  rdbReader.h \
  rdbTiledRdbOutputReceiver.h \
  rdbUtils.h \
  rdbCommon.h

INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db

