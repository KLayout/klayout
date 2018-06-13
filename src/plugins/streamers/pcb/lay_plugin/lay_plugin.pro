
TARGET = pcb
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$PWD/../db_plugin
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -lpcb

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
  extGerberImportDialog.h \

SOURCES = \
  extGerberImport.cc \
  extGerberImportDialog.cc \

FORMS = \
  GerberImportDialog.ui \
