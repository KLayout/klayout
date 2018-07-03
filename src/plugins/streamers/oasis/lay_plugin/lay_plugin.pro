
TARGET = oasis_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$PWD/../db_plugin
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -loasis

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
  layOASISReaderPlugin.h \
  layOASISWriterPlugin.h \

SOURCES = \
  layOASISReaderPlugin.cc \
  layOASISWriterPlugin.cc \

FORMS = \
  OASISWriterOptionPage.ui \
