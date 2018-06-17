

DESTDIR = $$OUT_PWD/../../..
TARGET = klayout_QtWidgets

include($$PWD/../../../lib.pri)

DEFINES += MAKE_GSI_QTWIDGETS_LIBRARY

INCLUDEPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_qtbasic

# because QPainter is used as base class for QStylePainter, we need this:
LIBS += -lklayout_QtGui

# because QObject is the base class of some classes we need this
LIBS += -lklayout_QtCore

SOURCES += \
  gsiDeclQtWidgetsAdd.cc

HEADERS += \

include(QtWidgets.pri)

