
DESTDIR = $$OUT_PWD/..
TARGET = klayout_lym

include($$PWD/../klayout.pri)
include($$PWD/../lib.pri)

DEFINES += MAKE_LYM_LIBRARY

SOURCES = \
  gsiDeclLymMacro.cc \
  lymMacroInterpreter.cc \
  lymMacro.cc \

HEADERS = \
  lymCommon.h \
  lymMacroInterpreter.h \
  lymMacro.h \

INCLUDEPATH += ../tl ../gsi ../rba ../pya
DEPENDPATH += ../tl ../gsi ../rba ../pya
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_rba -lklayout_pya

