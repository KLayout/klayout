
TARGET = bool_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

HEADERS = \
  layBooleanOperationsDialogs.h

SOURCES = \
  layBooleanOperationsDialogs.cc \
  layBooleanOperationsPlugin.cc

FORMS = \
  BooleanOptionsDialog.ui \
  SizingOptionsDialog.ui \
  MergeOptionsDialog.ui \
