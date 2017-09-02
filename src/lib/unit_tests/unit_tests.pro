
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = lib_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  libBasicTests.cc \

INCLUDEPATH += $$LIB_INC $$TL_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$LIB_INC $$TL_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_lib -lklayout_db -lklayout_tl -lklayout_gsi

