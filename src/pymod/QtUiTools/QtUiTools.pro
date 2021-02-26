
TARGET = QtUiTools

include($$PWD/../pymod.pri)

SOURCES = \
  QtUiToolsMain.cc \

HEADERS += \

contains(QT_MODULES, uitools) {
  LIBS += -lklayout_QtUiTools 
}

LIBS += -lklayout_QtCore

