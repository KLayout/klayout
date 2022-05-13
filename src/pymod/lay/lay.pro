
TARGET = laycore
REALMODULE = lay

include($$PWD/../pymod.pri)

SOURCES = \
  layMain.cc \

HEADERS += \

equals(HAVE_QT, "0") {
  LIBS += -lklayout_laybasic
} else {
  LIBS += -lklayout_lay
}

# hard linked as they contribute GSI classes to "lay" module:
LIBS += -lklayout_img -lklayout_edt -lklayout_ant -lklayout_lym

