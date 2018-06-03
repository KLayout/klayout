
TARGET = QtCore

include($$PWD/../pymod.pri)

SOURCES = \
  QtCoreMain.cc \

HEADERS += \

LIBS += -lklayout_QtCore -lklayout_QtGui -lklayout_QtWidgets
