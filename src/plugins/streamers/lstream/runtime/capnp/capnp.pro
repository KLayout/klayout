# DO NOT EDIT!
# (SEE ../capnp.pro)


TEMPLATE = lib
DESTDIR = $$OUT_PWD/../../../../..
INCLUDEPATH =

include($$PWD/../../lstream.pri)
include($$PWD/capnp.pri)

TARGET = xcapnp
CONFIG += staticlib
SOURCES = $$CAPNP_SOURCES
HEADERS = $$CAPNP_HEADERS

DEFINES += CAPNP_LITE
LIBS = 
QT = 

