
TARGET = rdbcore
REALMODULE = rdb

include($$PWD/../pymod.pri)

SOURCES = \
  rdbMain.cc \

HEADERS += \

LIBS += -lklayout_rdb
