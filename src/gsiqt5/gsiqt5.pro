
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)
include(qtdecl.pro)

DEFINES += MAKE_GSIQT_LIBRARY

TEMPLATE = lib

SOURCES += \
  gsiQt.cc \
  gsiQtBasic.cc \
  gsiQtAdditional.cc \

HEADERS += \
  gsiQt.h \
    gsiQtCommon.h

# NOTE: db is required since some bridges to db are provided (i.e db::Polygon)
INCLUDEPATH += ../tl ../gsi ../db
DEPENDPATH += ../tl ../gsi ../db
LIBS += -L$$DESTDIR -ltl -lgsi -ldb

