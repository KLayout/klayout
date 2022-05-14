
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = layui_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  layAbstractMenuTests.cc \

SOURCES = \

!equals(HAVE_QT, "0") {

  SOURCES += \
    layNetlistBrowserModelTests.cc \
    layNetlistBrowserTreeModelTests.cc \

}

INCLUDEPATH += $$LAYUI_INC $$LAYBASIC_INC $$TL_INC $$UI_INC $$DB_INC $$RDB_INC $$GSI_INC $$OUT_PWD/../ui
DEPENDPATH += $$LAYUI_INC $$LAYBASIC_INC $$TL_INC $$UI_INC $$DB_INC $$RDB_INC $$GSI_INC $$OUT_PWD/../ui

LIBS += -L$$DESTDIR_UT -lklayout_laybasic -lklayout_layui -lklayout_db -lklayout_tl -lklayout_gsi

