

DESTDIR = $$OUT_PWD/../../..
TARGET = klayout_QtGui

include($$PWD/../../../lib.pri)

DEFINES += MAKE_GSI_QTGUI_LIBRARY

INCLUDEPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_qtbasic

# there are some bridges between db and Qt, hence this:
INCLUDEPATH += $$DB_INC
DEPENDPATH += $$DB_INC
LIBS += -lklayout_db

# because QQbject is used as base class for many classes, we need this:
LIBS += -lklayout_QtCore

SOURCES += \
  gsiDeclQtGuiAdd.cc

HEADERS += \

include(QtGui.pri)
