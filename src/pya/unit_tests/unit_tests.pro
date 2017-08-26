
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = pya_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  pya.cc

INCLUDEPATH += $$GSI_TEST_INC $$PYA_INC $$DB_INC $$TL_INC $$GSI_INC $$UT_INC
DEPENDPATH += $$GSI_TEST_INC $$PYA_INC $$DB_INC $$TL_INC $$GSI_INC $$UT_INC

LIBS += -L$$DESTDIR_UT -lgsi_test -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_ut

