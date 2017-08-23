
DESTDIR = $$OUT_PWD/..
TARGET = klayout_ut

include($$PWD/../klayout.pri)

DEFINES += MAKE_UT_LIBRARY

TEMPLATE = lib

# Input
HEADERS = \
  utHead.h \
  utTestBase.h \
  utTestConsole.h \
  utCommon.h

SOURCES = \
  utMain.cc \
  utTestConsole.cc \
  utTestBase.cc \

INCLUDEPATH = ../tl ../db ../gsi ../lay ../ext ../lib
DEPENDPATH = ../tl ../db ../gsi ../lay ../ext ../lib

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi -lklayout_lay -lklayout_ext -lklayout_lib

equals(HAVE_RUBY, "1") {
  INCLUDEPATH += ../rba
  DEPENDPATH += ../rba
  LIBS += -lklayout_rba
} else {
  INCLUDEPATH += ../rbastub
  DEPENDPATH += ../rbastub
  LIBS += -lklayout_rbastub
}

equals(HAVE_PYTHON, "1") {
  INCLUDEPATH += ../pya
  DEPENDPATH += ../pya
  LIBS += -lklayout_pya
} else {
  INCLUDEPATH += ../pyastub
  DEPENDPATH += ../pyastub
  LIBS += -lklayout_pyastub
}

