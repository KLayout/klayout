
DESTDIR = $$OUT_PWD/..
TARGET = klayout_rbastub

include($$PWD/../klayout.pri)
include($$PWD/../lib.pri)

DEFINES += MAKE_RBA_LIBRARY

HEADERS = rbaCommon.h

SOURCES = rba.cc

INCLUDEPATH += ../tl ../gsi
DEPENDPATH += ../tl ../gsi
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi
  
