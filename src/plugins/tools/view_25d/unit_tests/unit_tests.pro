
DESTDIR_UT = $$OUT_PWD/../../../..

TARGET = view_25d_tests

include($$PWD/../../../../lib_ut.pri)

SOURCES = \
  layD25MemChunksTests.cc \
    layD25ViewUtilsTests.cc \
    layD25CameraTests.cc

INCLUDEPATH += $$LAY_INC $$TL_INC $$DB_INC $$GSI_INC $$PWD/../lay_plugin $$PWD/../../../common
DEPENDPATH += $$LAY_INC $$TL_INC $$DB_INC $$GSI_INC $$PWD/../lay_plugin $$PWD/../../../common

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi

PLUGINPATH = $$OUT_PWD/../../../../lay_plugins
QMAKE_RPATHDIR += $$PLUGINPATH

LIBS += -L$$PLUGINPATH -ld25_ui
