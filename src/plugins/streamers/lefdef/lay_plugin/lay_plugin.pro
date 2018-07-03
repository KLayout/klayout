
TARGET = lefdef_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$PWD/../db_plugin
DEPENDPATH += $$PWD/../db_plugin
LIBS += -L$$DESTDIR/../db_plugins -llefdef

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH/db_plugins
}

HEADERS = \
    layLEFDEFImportDialogs.h

SOURCES = \
    layLEFDEFPlugin.cc \
    layLEFDEFImportDialogs.cc \
    layLEFDEFImport.cc

FORMS = \
  LEFDEFImportOptionsDialog.ui \
  LEFDEFTechnologyComponentEditor.ui \
