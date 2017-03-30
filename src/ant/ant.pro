
DESTDIR = $$OUT_PWD/..
TARGET = klayout_ant

include($$PWD/../klayout.pri)

DEFINES += MAKE_ANT_LIBRARY

TEMPLATE = lib

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

INCLUDEPATH += ../tl ../gsi ../laybasic ../db
DEPENDPATH += ../tl ../gsi ../laybasic ../db
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_db

