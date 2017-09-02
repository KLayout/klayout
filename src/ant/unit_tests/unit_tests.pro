
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = ant_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  antBasicTests.cc \

INCLUDEPATH += $$ANT_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$ANT_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_ant -lklayout_laybasic -lklayout_db -lklayout_tl -lklayout_gsi

