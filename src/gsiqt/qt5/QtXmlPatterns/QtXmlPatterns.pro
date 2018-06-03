

DESTDIR = $$OUT_PWD/../../..
TARGET = klayout_QtXmlPatterns

include($$PWD/../../../lib.pri)

DEFINES += MAKE_GSI_QTXMLPATTERNS_LIBRARY

INCLUDEPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_qtbasic

# because QQbject is used as base class for many classes, we need this:
LIBS += -lklayout_QtCore

SOURCES += \

HEADERS += \

include(QtXmlPatterns.pri)

