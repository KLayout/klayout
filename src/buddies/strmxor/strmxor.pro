
include($$PWD/../../klayout.pri)

TEMPLATE = app

TARGET = strmxor
DESTDIR = $$OUT_PWD/../..

SOURCES = strmxor.cc

INCLUDEPATH += ../../tl ../../db ../../gsi
DEPENDPATH += ../../tl ../../db ../../gsi
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi
