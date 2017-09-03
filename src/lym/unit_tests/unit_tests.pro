
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = lym_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  lymBasicTests.cc \

INCLUDEPATH += $$LYM_INC $$TL_INC $$GSI_INC
DEPENDPATH += $$LYM_INC $$TL_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_lym -lklayout_tl -lklayout_gsi

