
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = rba_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  rba.cc

INCLUDEPATH += $$GSI_TEST_INC $$RBA_INC $$TL_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$GSI_TEST_INC $$RBA_INC $$TL_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lgsi_test -lklayout_tl -lklayout_db -lklayout_gsi

