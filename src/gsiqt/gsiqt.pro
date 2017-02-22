
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)
include(qtdecl.pro)
include($$PWD/../gsiqt_common/gsiqt_common.pri)

DEFINES += MAKE_GSIQT_LIBRARY

TEMPLATE = lib

# NOTE: db is required since some bridges to db are provided (i.e db::Polygon)
INCLUDEPATH += ../tl ../gsi ../db ../gsiqt_common
DEPENDPATH += ../tl ../gsi ../db ../gsiqt_common

LIBS += -L$$DESTDIR -ltl -lgsi -ldb

