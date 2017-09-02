
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = lay_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  laySalt.cc \

INCLUDEPATH += $$LAY_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$LAY_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_lay -lklayout_laybasic -lklayout_db -lklayout_tl -lklayout_gsi

