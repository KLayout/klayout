
TARGET = d25_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$RDB_INC $$ANT_INC
DEPENDPATH += $$RDB_INC $$ANT_INC
LIBS += -L$$DESTDIR/.. -lklayout_rdb -lklayout_ant

HEADERS = \
  layD25View.h \
  layD25ViewWidget.h \

SOURCES = \
  layD25View.cc \
  layD25ViewWidget.cc \
  layD25Plugin.cc

FORMS = \
  D25View.ui \
