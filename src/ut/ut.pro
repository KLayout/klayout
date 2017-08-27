
DESTDIR = $$OUT_PWD/..
TARGET = klayout_ut

include($$PWD/../lib.pri)

DEFINES += MAKE_UT_LIBRARY

# Input
HEADERS = \
  utHead.h \
  utTestBase.h \
  utTestConsole.h \
  utCommon.h

SOURCES = \
  utTestConsole.cc \
  utTestBase.cc \

INCLUDEPATH = $$TL_INC $$DB_INC $$GSI_INC
DEPENDPATH = $$TL_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi

INCLUDEPATH += $$RBA_INC
DEPENDPATH += $$RBA_INC

equals(HAVE_RUBY, "1") {
  LIBS += -lklayout_rba
} else {
  LIBS += -lklayout_rbastub
}

INCLUDEPATH += $$PYA_INC
DEPENDPATH += $$PYA_INC

equals(HAVE_PYTHON, "1") {
  LIBS += -lklayout_pya
} else {
  LIBS += -lklayout_pyastub
}

