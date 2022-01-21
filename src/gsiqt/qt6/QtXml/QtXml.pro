

DESTDIR = $$OUT_PWD/../../..
TARGET = klayout_QtXml

include($$PWD/../../../lib.pri)

DEFINES += MAKE_GSI_QTXML_LIBRARY

INCLUDEPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_qtbasic

SOURCES += \

HEADERS += \

include(QtXml.pri)

