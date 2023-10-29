
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = lay_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  laySalt.cc \
  layHelpIndexTest.cc \
  laySaltParsedURLTests.cc \
  laySessionTests.cc

INCLUDEPATH += $$LAY_INC $$TL_INC $$LAYBASIC_INC $$LAYUI_INC $$LAYVIEW_INC $$DB_INC $$GSI_INC $$ANT_INC $$IMG_INC $$RDB_INC
DEPENDPATH += $$LAY_INC $$TL_INC $$LAYBASIC_INC $$LAYUI_INC $$LAYVIEW_INC $$DB_INC $$GSI_INC $$ANT_INC $$IMG_INC $$RDB_INC

LIBS += -L$$DESTDIR_UT -lklayout_lay -lklayout_laybasic -lklayout_layui -lklayout_layview -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_ant -lklayout_img -lklayout_rdb

