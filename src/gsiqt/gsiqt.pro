
DESTDIR = $$OUT_PWD/..
TARGET = klayout_gsiqt

include($$PWD/../lib.pri)

DEFINES += MAKE_GSIQT_LIBRARY

# NOTE: db is required since some bridges to db are provided (i.e db::Polygon)
INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC $$GSIQT_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC $$GSIQT_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db

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

