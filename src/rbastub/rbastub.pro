
DESTDIR = $$OUT_PWD/..
TARGET = klayout_rbastub

include($$PWD/../lib.pri)

DEFINES += MAKE_RBA_LIBRARY

HEADERS = rbaCommon.h rba.h

SOURCES = rba.cc

INCLUDEPATH += $$TL_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$GSI_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi
  
