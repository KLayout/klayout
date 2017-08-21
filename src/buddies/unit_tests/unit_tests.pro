
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = bd_tests

include($$PWD/../../klayout.pri)
include($$PWD/../../lib_ut.pri)

SOURCES = \
  bdBasicTests.cc \
  bdConverterTests.cc \
    bdStrm2txtTests.cc \
    bdStrmclipTests.cc \
    bdStrmcmpTests.cc \
    bdStrmxorTests.cc \


INCLUDEPATH += ../src/bd ../../db ../../tl ../../gsi ../../laybasic ../../lay ../../ut
DEPENDPATH += ../src/bd ../../db ../../tl ../../gsi ../../laybasic ../../lay ../../ut

LIBS += -L$$DESTDIR_UT -lklayout_bd -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_lay -lklayout_ut
