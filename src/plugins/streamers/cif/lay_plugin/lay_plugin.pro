
TARGET = cif_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$PWD/../db_plugin
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -lcif

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
  layCIFReaderPlugin.h \
  layCIFWriterPlugin.h \

SOURCES = \
  layCIFReaderPlugin.cc \
  layCIFWriterPlugin.cc \

FORMS = \
  CIFWriterOptionPage.ui \
  CIFReaderOptionPage.ui \
