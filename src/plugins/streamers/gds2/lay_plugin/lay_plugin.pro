
TARGET = gds2_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$PWD/../db_plugin
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -lgds2

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
  layGDS2ReaderPlugin.h \
  layGDS2WriterPlugin.h \

SOURCES = \
  layGDS2ReaderPlugin.cc \
  layGDS2WriterPlugin.cc \

FORMS = \
  GDS2ReaderOptionPage.ui \
  GDS2WriterOptionPage.ui \
