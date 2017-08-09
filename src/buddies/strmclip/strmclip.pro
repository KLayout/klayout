
include($$PWD/../../klayout.pri)

TEMPLATE = app

TARGET = strmclip
DESTDIR = $$OUT_PWD/../..

SOURCES = strmclip.cc

INCLUDEPATH += ../../tl ../../db ../../gsi
DEPENDPATH += ../../tl ../../db ../../gsi
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi
