
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_bd

include($$PWD/../../klayout.pri)

DEFINES += MAKE_BD_LIBRARY

TEMPLATE = lib

SOURCES = \
  bdInit.cc \
  bdReaderOptions.cc \
  bdWriterOptions.cc \

HEADERS = \
  bdCommon.h \
  bdInit.h \
  bdReaderOptions.h \
  bdWriterOptions.h \

RESOURCES = \

INCLUDEPATH += ../../tl ../../db ../../gsi ../../version
DEPENDPATH += ../../tl ../../db ../../gsi ../../version
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi

