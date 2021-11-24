
TARGET = QtCore

include($$PWD/../pymod.pri)

SOURCES = \
  QtCoreMain.cc \

HEADERS += \

LIBS += -lklayout_QtCore -lklayout_QtGui

greaterThan(QT_MAJOR_VERSION, 4) {
  LIBS += -lklayout_QtWidgets
}
