
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_qtbasic

include($$PWD/../../lib.pri)

DEFINES += MAKE_GSI_QTBASIC_LIBRARY

# NOTE: db is required since some bridges to db are provided (i.e db::Polygon)
INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC $$QTBASIC_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db

SOURCES += \
  gsiQt.cc \
  gsiQtHelper.cc

HEADERS += \
  gsiDeclQtAllTypeTraits.h \
  gsiQt.h \
  gsiQtBasicCommon.h \ \
  gsiQtCoreExternals.h \
  gsiQtDesignerExternals.h \
  gsiQtGuiExternals.h \
  gsiQtHelper.h \
  gsiQtMultimediaExternals.h \
  gsiQtNetworkExternals.h \
  gsiQtPrintSupportExternals.h \
  gsiQtSqlExternals.h \
  gsiQtSvgExternals.h \
  gsiQtUiToolsExternals.h \
  gsiQtWidgetsExternals.h \
  gsiQtXmlExternals.h \
  gsiQtXmlPatternsExternals.h
