
include($$PWD/../klayout.pri)

TEMPLATE = lib

INCLUDEPATH += $$DB_INC $$TL_INC $$GSI_INC $$PWD/common
DEPENDPATH += $$DB_INC $$TL_INC $$GSI_INC $$PWD/common
LIBS += -L$$DESTDIR/.. -lklayout_db -lklayout_tl -lklayout_gsi

DEFINES += MAKE_DB_PLUGIN_LIBRARY
