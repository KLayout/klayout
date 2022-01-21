

DESTDIR = $$OUT_PWD/../../..
TARGET = klayout_QtPrintSupport

include($$PWD/../../../lib.pri)

DEFINES += MAKE_GSI_QTPRINTSUPPORT_LIBRARY

INCLUDEPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_qtbasic

# because QQbject is used as base class for many classes, we need this:
LIBS += -lklayout_QtCore

# because QWidget is used for some UI stuff, we need this:
LIBS += -lklayout_QtWidgets

# because QPagedPaintDevice is used, we need this:
LIBS += -lklayout_QtGui

SOURCES += \

HEADERS += \

include(QtPrintSupport.pri)

