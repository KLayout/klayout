
TEMPLATE = lib
DESTDIR = $$OUT_PWD/../../../../..

include($$PWD/../../lstream.pri)
include($$PWD/capnp.pri)

TARGET = xcapnp
SOURCES = $$CAPNP_SOURCES
HEADERS = $$CAPNP_HEADERS

DEFINES += CAPNP_LITE
INCLUDEPATH = 
LIBS = 
QT = 

