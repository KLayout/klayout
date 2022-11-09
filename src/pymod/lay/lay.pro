
TARGET = laycore
REALMODULE = lay
PYI = laycore.pyi

include($$PWD/../pymod.pri)

SOURCES = \
  layMain.cc \

HEADERS += \

LIBS += -lklayout_layview

!equals(HAVE_QT, "0") {
  LIBS += -lklayout_layui
  LIBS += -lklayout_lay
}

# hard linked as they contribute GSI classes to "lay" module:
LIBS += -lklayout_laybasic -lklayout_img -lklayout_edt -lklayout_ant -lklayout_lym

