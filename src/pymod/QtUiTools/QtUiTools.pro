
TARGET = QtUiTools

include($$PWD/../pymod.pri)

SOURCES = \
  QtUiToolsMain.cc \

HEADERS += \

LIBS += -lklayout_QtUiTools 

contains(QT_MODULES, uitools) {
  LIBS += -lklayout_QtCore
}
