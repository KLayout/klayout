
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

DEFINES += MAKE_UT_LIBRARY

TEMPLATE = lib

# Input
HEADERS = \
  utHead.h \
    utCommon.h

SOURCES = \
  utMain.cc \

INCLUDEPATH = ../tl ../db ../gsi ../lay ../ext ../lib
DEPENDPATH = ../tl ../db ../gsi ../lay ../ext ../lib

LIBS += -L$$DESTDIR -ltl -ldb -lgsi -llay -lext -llib

equals(HAVE_RUBY, "1") {
  INCLUDEPATH += ../rba
  DEPENDPATH += ../rba
  LIBS += -lrba
} else {
  INCLUDEPATH += ../rbastub
  DEPENDPATH += ../rbastub
  LIBS += -lrbastub
}

equals(HAVE_PYTHON, "1") {
  INCLUDEPATH += ../pya
  DEPENDPATH += ../pya
  LIBS += -lpya
} else {
  INCLUDEPATH += ../pyastub
  DEPENDPATH += ../pyastub
  LIBS += -lpyastub
}

