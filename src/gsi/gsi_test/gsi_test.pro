
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_gsi_test

include($$PWD/../../lib.pri)

# don't install this test library
INSTALLS =

DEFINES += MAKE_GSI_TEST_LIBRARY

SOURCES = \
  gsiTest.cc \

HEADERS = \
  gsiTest.h \
  gsiTestForceLink.h \

INCLUDEPATH += $$TL_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$GSI_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi
