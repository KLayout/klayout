
DESTDIR = $$OUT_PWD/..
TARGET = klayout_pyastub

include($$PWD/../klayout.pri)
include($$PWD/../lib.pri)

DEFINES += MAKE_PYA_LIBRARY

SOURCES = pya.cc 

INCLUDEPATH += ../tl ../gsi
DEPENDPATH += ../tl ../gsi
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi

HEADERS += \
    pyaCommon.h

