
TARGET = xor_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$RDB_INC $$ANT_INC
DEPENDPATH += $$RDB_INC $$ANT_INC
LIBS += -L$$DESTDIR/.. -lklayout_rdb -lklayout_ant

HEADERS = \
  layXORToolDialog.h \
  layXORProgress.h

SOURCES = \
  layXORToolDialog.cc \
  layXORProgress.cc \
  layXORPlugin.cc

FORMS = \
  XORToolDialog.ui \
