
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)
include($$PWD/../lstream.pri)

INCLUDEPATH += $$PWD/../db_plugin $$PWD/../db_plugin/capnp
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -llstream

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
  layLStreamReaderPlugin.h \
  layLStreamWriterPlugin.h \

SOURCES = \
  layLStreamReaderPlugin.cc \
  layLStreamWriterPlugin.cc \

FORMS = \
  LStreamWriterOptionPage.ui \

