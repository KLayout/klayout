
TARGET = dxf_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$PWD/../db_plugin
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -ldxf

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
  layDXFReaderPlugin.h \
  layDXFWriterPlugin.h \

SOURCES = \
  layDXFReaderPlugin.cc \
  layDXFWriterPlugin.cc \

FORMS = \
  DXFReaderOptionPage.ui \
  DXFWriterOptionPage.ui \

