
TARGET = QtCore

include($$PWD/../pymod.pri)

SOURCES = \
  QtCoreMain.cc \

HEADERS += \

LIBS += -lklayout_QtCore -lklayout_QtGui

equals(HAVE_QT5, "1") {
  LIBS += -lklayout_QtWidgets
}
