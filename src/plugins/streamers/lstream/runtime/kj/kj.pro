
TEMPLATE = lib
DESTDIR = $$OUT_PWD/../../../../..

include($$PWD/../../lstream.pri)
include($$PWD/kj.pri)

TARGET = xkj
SOURCES = $$KJ_SOURCES
HEADERS = $$KJ_HEADERS

DEFINES = CAPNP_LITE
INCLUDEPATH =
LIBS = 
QT = 

