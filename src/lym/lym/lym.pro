
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_lym

include($$PWD/../../lib.pri)

DEFINES += MAKE_LYM_LIBRARY

SOURCES = \
  gsiDeclLymMacro.cc \
  lymMacroInterpreter.cc \
  lymMacro.cc \

HEADERS = \
  lymCommon.h \
  lymInclude.h \
  lymMacroInterpreter.h \
  lymMacro.h \

INCLUDEPATH += $$TL_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$GSI_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi

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
