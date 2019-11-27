
TARGET = mag_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$PWD/../db_plugin
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -lmag

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
  layMAGReaderPlugin.h \
  layMAGWriterPlugin.h \

SOURCES = \
  layMAGReaderPlugin.cc \
  layMAGWriterPlugin.cc \

FORMS = \
  MAGWriterOptionPage.ui \
  MAGReaderOptionPage.ui \
