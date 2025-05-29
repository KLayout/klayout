
TARGET = pexcore
REALMODULE = pex
PYI = pexcore.pyi

include($$PWD/../pymod.pri)

SOURCES = \
  pexMain.cc \

HEADERS += \

LIBS += -lklayout_pex

