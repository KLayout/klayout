
TARGET = libcore
REALMODULE = lib
PYI = libcore.pyi

include($$PWD/../pymod.pri)

SOURCES = \
  libMain.cc \

HEADERS += \

LIBS += -lklayout_lib

