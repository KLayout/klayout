
TARGET = dbcore
REALMODULE = db

include($$PWD/../pymod.pri)

SOURCES = \
  dbMain.cc \

HEADERS += \

LIBS += -lklayout_db

