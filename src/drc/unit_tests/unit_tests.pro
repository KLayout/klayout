
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = drc_tests

include($$PWD/../../klayout.pri)
include($$PWD/../../lib_ut.pri)

SOURCES = \
  drcBasicTests.cc \

INCLUDEPATH += ../drc ../../rdb ../../db ../../tl ../../gsi ../../lym ../../ut
DEPENDPATH += ../drc ../../rdb ../../db ../../tl ../../gsi ../../lym ../../ut

LIBS += -L$$DESTDIR_UT -lklayout_drc -lklayout_rdb -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_lym -lklayout_ut
