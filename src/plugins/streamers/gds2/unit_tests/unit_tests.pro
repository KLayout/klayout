
DESTDIR_UT = $$OUT_PWD/../../../..

TARGET = gds2_tests

include($$PWD/../../../../lib_ut.pri)

SOURCES = \
  dbGDS2ReaderTests.cc \
  dbGDS2WriterTests.cc

INCLUDEPATH += $$LAY_INC $$TL_INC $$DB_INC $$GSI_INC $$PWD/../db_plugin $$PWD/../../../common
DEPENDPATH += $$LAY_INC $$TL_INC $$DB_INC $$GSI_INC $$PWD/../db_plugin $$PWD/../../../common

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi

PLUGINPATH = $$OUT_PWD/../../../../db_plugins
QMAKE_RPATHDIR += $$PLUGINPATH

LIBS += -L$$PLUGINPATH -lgds2
