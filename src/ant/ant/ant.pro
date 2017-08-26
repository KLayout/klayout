
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_ant

include($$PWD/../../lib.pri)

DEFINES += MAKE_ANT_LIBRARY

HEADERS = \
  antConfig.h \
  antConfigPage.h \
  antObject.h \
  antPlugin.h \
  antPropertiesPage.h \
  antService.h \
  antTemplate.h \
    antForceLink.h \
    antCommon.h

FORMS = \
  RulerConfigPage.ui \
  RulerConfigPage2.ui \
  RulerConfigPage3.ui \
  RulerConfigPage4.ui \
  RulerPropertiesPage.ui \

SOURCES = \
  antConfig.cc \
  antConfigPage.cc \
  antObject.cc \
  antPlugin.cc \
  antPropertiesPage.cc \
  antService.cc \
  antTemplate.cc \
  gsiDeclAnt.cc \
    antForceLink.cc

INCLUDEPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_db

