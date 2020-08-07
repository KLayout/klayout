
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = edt_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  edtBasicTests.cc \
    edtDistributeTests.cc

INCLUDEPATH += $$EDT_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$EDT_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_edt -lklayout_laybasic -lklayout_db -lklayout_tl -lklayout_gsi

