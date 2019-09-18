
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
  bdStrmrunTests.cc \

equals(HAVE_RUBY, "1") {

# the basic tests require Ruby as they use Ruby unittest
SOURCES += \
  buddies_main.cc \

}

INCLUDEPATH += $$BD_INC $$DB_INC $$TL_INC $$GSI_INC $$RBA_INC
DEPENDPATH += $$BD_INC $$DB_INC $$TL_INC $$GSI_INC $$RBA_INC

LIBS += -L$$DESTDIR_UT -lklayout_bd -lklayout_db -lklayout_tl -lklayout_gsi -l$$RBA_LIB
