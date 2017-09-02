
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = gsi_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  gsiExpression.cc \

INCLUDEPATH += $$TL_INC $$GSI_INC $$GSI_TEST_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$GSI_TEST_INC $$DB_INC

LIBS += -L$$DESTDIR_UT -lklayout_tl -lklayout_gsi -lgsi_test -lklayout_db

