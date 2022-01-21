

DESTDIR = $$OUT_PWD/../../..
TARGET = klayout_QtCore

include($$PWD/../../../lib.pri)

DEFINES += MAKE_GSI_QTCORE_LIBRARY

# NOTE: db is required since some bridges to db are provided (i.e db::Polygon)
INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC $$QTBASIC_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_qtbasic

SOURCES += \
  gsiDeclQtCoreAdd.cc

HEADERS += \

include(QtCore.pri)
