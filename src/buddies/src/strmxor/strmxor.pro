
include($$PWD/../../../klayout.pri)

TEMPLATE = app

TARGET = strmxor
DESTDIR = $$OUT_PWD/../../..

SOURCES = strmxor.cc

INCLUDEPATH += ../bd ../../../tl ../../../db ../../../gsi
DEPENDPATH += ../bd ../../../tl ../../../db ../../../gsi
LIBS += -L$$DESTDIR -lklayout_bd -lklayout_tl -lklayout_db -lklayout_gsi
