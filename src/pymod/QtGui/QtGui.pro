
TARGET = QtGui

include($$PWD/../pymod.pri)

SOURCES = \
  QtGuiMain.cc \

HEADERS += \

LIBS += -lklayout_QtGui -lklayout_QtCore

greaterThan(QT_MAJOR, "4") {
  LIBS += -lklayout_QtWidgets
}

