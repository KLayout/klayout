
include($$PWD/../../klayout.pri)

TEMPLATE = app

TARGET = strmcmp
DESTDIR = $$OUT_PWD/../..

SOURCES = strmcmp.cc

INCLUDEPATH += ../bd ../../tl ../../db ../../gsi
DEPENDPATH += ../bd ../../tl ../../db ../../gsi
LIBS += -L$$DESTDIR -lklayout_bd -lklayout_tl -lklayout_db -lklayout_gsi
