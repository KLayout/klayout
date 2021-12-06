
TARGET = QtCore5Compat

include($$PWD/../pymod.pri)

SOURCES = \
  QtCore5CompatMain.cc \

HEADERS += \

LIBS += -lklayout_QtCore5Compat -lklayout_QtNetwork
