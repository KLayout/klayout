
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_lvs

include($$PWD/../../lib.pri)

DEFINES += MAKE_LVS_LIBRARY

SOURCES = \
  lvsForceLink.cc \

HEADERS = \
  lvsCommon.h \
  lvsForceLink.h \

!equals(HAVE_QT, "0") {
  RESOURCES = \
    lvsResources.qrc
}

INCLUDEPATH += $$TL_INC $$DB_INC $$GSI_INC $$LYM_INC $$RDB_INC
DEPENDPATH += $$TL_INC $$DB_INC $$GSI_INC $$LYM_INC $$RDB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi -lklayout_lym -lklayout_rdb -lklayout_drc

