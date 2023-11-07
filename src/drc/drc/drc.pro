
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_drc

include($$PWD/../../lib.pri)

DEFINES += MAKE_DRC_LIBRARY

SOURCES = \
  drcForceLink.cc \

HEADERS = \
  drcCommon.h \
  drcForceLink.h \

!equals(HAVE_QT, "0") {
  RESOURCES = \
    drcResources.qrc
}

INCLUDEPATH += $$TL_INC $$DB_INC $$GSI_INC $$LYM_INC $$RDB_INC
DEPENDPATH += $$TL_INC $$DB_INC $$GSI_INC $$LYM_INC $$RDB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi -lklayout_lym -lklayout_rdb

