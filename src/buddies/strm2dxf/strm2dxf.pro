
include($$PWD/../../klayout.pri)

TEMPLATE = app

TARGET = strm2dxf
DESTDIR = $$OUT_PWD/../..

SOURCES = strm2dxf.cc

INCLUDEPATH += ../../tl ../../db ../../gsi
DEPENDPATH += ../../tl ../../db ../../gsi
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi
