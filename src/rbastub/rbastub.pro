
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

DEFINES += MAKE_RBA_LIBRARY

TEMPLATE = lib

HEADERS = rbaCommon.h

SOURCES = rba.cc

INCLUDEPATH += ../tl ../gsi
DEPENDPATH += ../tl ../gsi
LIBS += -L$$DESTDIR -ltl -lgsi
  
