
TARGET = QtUiTools

include($$PWD/../pymod.pri)

SOURCES = \
  QtUiToolsMain.cc \

HEADERS += \

equals(HAVE_QT_UITOOLS, "1") {
  LIBS += -lklayout_QtUiTools 
}

LIBS += -lklayout_QtCore

