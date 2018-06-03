
TARGET = QtGui

include($$PWD/../pymod.pri)

SOURCES = \
  QtGuiMain.cc \

HEADERS += \

LIBS += -lklayout_QtGui -lklayout_QtCore

equals(HAVE_QT5, "1") {
  LIBS += -lklayout_QtWidgets
}

