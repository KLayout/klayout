
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = layview_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  layLayoutViewTests.cc \

INCLUDEPATH += $$TL_INC $$LAYBASIC_INC $$LAYVIEW_INC $$LAYUI_INC $$DB_INC $$RDB_INC $$GSI_INC $$OUT_PWD/../layview
DEPENDPATH += $$TL_INC $$LAYBASIC_INC $$LAYVIEW_INC $$LAYUI_INC $$DB_INC $$RDB_INC $$GSI_INC $$OUT_PWD/../layview

LIBS += -L$$DESTDIR_UT -lklayout_layview -lklayout_db -lklayout_tl -lklayout_gsi

