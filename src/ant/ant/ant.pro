
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_ant

include($$PWD/../../lib.pri)

DEFINES += MAKE_ANT_LIBRARY

!equals(HAVE_QT, "0") {

FORMS = \
  RulerConfigPage.ui \
  RulerConfigPage2.ui \
  RulerConfigPage3.ui \
  RulerConfigPage4.ui \
  RulerPropertiesPage.ui \

}

# Disabled without Qt:

HEADERS = \
  antConfigPage.h \
  antPropertiesPage.h \

SOURCES = \
  antConfigPage.cc \
  antPropertiesPage.cc \

# Enabled without Qt:

HEADERS += \
  antConfig.h \
  antEditorOptionsPages.h \
  antObject.h \
  antPlugin.h \
  antService.h \
  antTemplate.h \
  antForceLink.h \
  antCommon.h

SOURCES += \
  antConfig.cc \
  antEditorOptionsPages.cc \
  antObject.cc \
  antPlugin.cc \
  antService.cc \
  antTemplate.cc \
  gsiDeclAnt.cc \
  antForceLink.cc

INCLUDEPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$LAYVIEW_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$LAYVIEW_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_layview -lklayout_db

!equals(HAVE_QT, "0") {

  INCLUDEPATH += $$LAYUI_INC
  DEPENDPATH += $$LAYUI_INC

  LIBS += -lklayout_layui

}

