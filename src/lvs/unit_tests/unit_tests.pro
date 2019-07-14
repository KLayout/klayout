
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = lvs_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  lvsBasicTests.cc \
  lvsSimpleTests.cc \
    lvsTests.cc

INCLUDEPATH += $$DRC_INC $$TL_INC $$RDB_INC $$DB_INC $$GSI_INC $$LYM_INC
DEPENDPATH += $$DRC_INC $$TL_INC $$RDB_INC $$DB_INC $$GSI_INC $$LYM_INC

LIBS += -L$$DESTDIR_UT -lklayout_lvs -lklayout_drc -lklayout_rdb -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_lym
