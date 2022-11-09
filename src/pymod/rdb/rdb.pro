
TARGET = rdbcore
REALMODULE = rdb
PYI = rdbcore.pyi

include($$PWD/../pymod.pri)

SOURCES = \
  rdbMain.cc \

HEADERS += \

LIBS += -lklayout_rdb
