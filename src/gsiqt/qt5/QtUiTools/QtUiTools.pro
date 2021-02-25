

DESTDIR = $$OUT_PWD/../../..
TARGET = klayout_QtUiTools

include($$PWD/../../../lib.pri)

DEFINES += MAKE_GSI_QTUITOOLS_LIBRARY

INCLUDEPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_qtbasic -lklayout_QtCore

SOURCES += \

HEADERS += \

include(QtUiTools.pri)

