
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

DEFINES += MAKE_PYA_LIBRARY

TEMPLATE = lib

SOURCES = pya.cc 

INCLUDEPATH += ../tl ../gsi
DEPENDPATH += ../tl ../gsi
LIBS += -L$$DESTDIR -ltl -lgsi

HEADERS += \
    pyaCommon.h

