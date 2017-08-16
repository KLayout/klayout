
include($$PWD/../../klayout.pri)

TEMPLATE = app

TARGET = strm2oas
DESTDIR = $$OUT_PWD/../..

SOURCES = strm2oas.cc

INCLUDEPATH += ../bd ../../tl ../../db ../../gsi
DEPENDPATH += ../bd ../../tl ../../db ../../gsi
LIBS += -L$$DESTDIR -lklayout_bd -lklayout_tl -lklayout_db -lklayout_gsi
