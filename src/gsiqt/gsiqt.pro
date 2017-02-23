
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

DEFINES += MAKE_GSIQT_LIBRARY

TEMPLATE = lib

# NOTE: db is required since some bridges to db are provided (i.e db::Polygon)
INCLUDEPATH += ../tl ../gsi ../db ../gsiqt
DEPENDPATH += ../tl ../gsi ../db ../gsiqt

LIBS += -L$$DESTDIR -ltl -lgsi -ldb

SOURCES += \
  gsiQt.cc \
  gsiDeclQtBasic.cc \
  gsiDeclQt5Basic.cc \
  gsiQtHelper.cc

HEADERS += \
  gsiQt.h \
  gsiQtCommon.h \
  gsiQtHelper.h

equals(HAVE_QT5, "1") {
  include($$PWD/../gsiqt5/qtdecl.pri)
} else {
  include($$PWD/../gsiqt4/qtdecl.pri)
}

gsiqt.depends += gsi
laybasic.depends += gsiqt
lay.depends += gsiqt 

