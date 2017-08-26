
DESTDIR = $$OUT_PWD/../..
TARGET = gsi_test

include($$PWD/../../lib.pri)

DEFINES += MAKE_GSI_LIBRARY

SOURCES = \
  gsiTest.cc \

HEADERS = \
  gsiTest.h \

INCLUDEPATH += $$TL_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$GSI_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi

