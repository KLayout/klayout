
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

HEADERS = \
  antConfigPage.h \
  antPropertiesPage.h \

SOURCES = \
  antConfigPage.cc \
  antPropertiesPage.cc \

}

HEADERS += \
  antConfig.h \
  antObject.h \
  antPlugin.h \
  antService.h \
  antTemplate.h \
  antForceLink.h \
  antCommon.h

SOURCES += \
  antConfig.cc \
  antObject.cc \
  antPlugin.cc \
  antService.cc \
  antTemplate.cc \
  gsiDeclAnt.cc \
  antForceLink.cc

INCLUDEPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_db

