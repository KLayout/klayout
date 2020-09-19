
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = lym_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  lymBasicTests.cc

INCLUDEPATH += $$RBA_INC $$PYA_INC $$LYM_INC $$TL_INC $$GSI_INC
DEPENDPATH += $$RBA_INC $$PYA_INC $$LYM_INC $$TL_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_lym -lklayout_tl -lklayout_gsi

equals(HAVE_RUBY, "1") {
  LIBS += -lklayout_rba
} else {
  LIBS += -lklayout_rbastub
}

equals(HAVE_PYTHON, "1") {
  LIBS += -lklayout_pya
} else {
  LIBS += -lklayout_pyastub
}

