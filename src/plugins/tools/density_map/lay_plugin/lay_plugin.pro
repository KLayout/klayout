
TARGET = density_map_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$IMG_INC $$ANT_INC
DEPENDPATH += $$IMG_INC $$ANT_INC
LIBS += -L$$DESTDIR/.. -lklayout_img -lklayout_ant

HEADERS = \
  layDensityMapDialog.h \

SOURCES = \
  layDensityMapDialog.cc \

FORMS = \
  DensityMapDialog.ui \
