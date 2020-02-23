
DESTDIR_UT = $$OUT_PWD/../../../..

TARGET = net_tracer_tests

include($$PWD/../../../../lib_ut.pri)

SOURCES = \
    dbTraceAllNets.cc \
    dbNetTracerTests.cc

INCLUDEPATH += $$LAY_INC $$TL_INC $$DB_INC $$GSI_INC $$PWD/../db_plugin $$PWD/../../../common
DEPENDPATH += $$LAY_INC $$TL_INC $$DB_INC $$GSI_INC $$PWD/../db_plugin $$PWD/../../../common

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi

# This makes the test pull the mebes library for testing (not installed)
PLUGINPATH = $$OUT_PWD/../../../../db_plugins
QMAKE_RPATHDIR += $$PLUGINPATH

LIBS += -L$$PLUGINPATH -lnet_tracer
