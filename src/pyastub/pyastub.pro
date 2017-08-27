
DESTDIR = $$OUT_PWD/..
TARGET = klayout_pyastub

include($$PWD/../lib.pri)

DEFINES += MAKE_PYA_LIBRARY

SOURCES = pya.cc 

INCLUDEPATH += $$TL_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$GSI_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi

HEADERS += \
    pyaCommon.h \
    pya.h

