
TEMPLATE = lib
DESTDIR = $$OUT_PWD/../../../../..
INCLUDEPATH =

include($$PWD/../../lstream.pri)
include($$PWD/kj.pri)

TARGET = xkj
CONFIG += staticlib
SOURCES = $$KJ_SOURCES
HEADERS = $$KJ_HEADERS

DEFINES = CAPNP_LITE
LIBS = 
QT = 

