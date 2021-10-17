
TARGET = QtCore

include($$PWD/../pymod.pri)

SOURCES = \
  QtCoreMain.cc \

HEADERS += \

LIBS += -lklayout_QtCore -lklayout_QtGui

greaterThan(QT_MAJOR, "4") {
  LIBS += -lklayout_QtWidgets
}
