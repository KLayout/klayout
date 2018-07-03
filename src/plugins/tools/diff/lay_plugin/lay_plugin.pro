
TARGET = diff_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$RDB_INC $$ANT_INC
DEPENDPATH += $$RDB_INC $$ANT_INC
LIBS += -L$$DESTDIR/.. -lklayout_rdb -lklayout_ant

HEADERS = \
  layDiffToolDialog.h

SOURCES = \
  layDiffPlugin.cc \
  layDiffToolDialog.cc

FORMS = \
  DiffToolDialog.ui \
