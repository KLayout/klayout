
TARGET = dbcore
REALMODULE = db
PYI = dbcore.pyi

include($$PWD/../pymod.pri)

SOURCES = \
  dbMain.cc \

HEADERS += \

LIBS += -lklayout_db

