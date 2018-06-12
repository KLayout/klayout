
include($$PWD/../klayout.pri)

TEMPLATE = lib

INCLUDEPATH += $$DB_INC $$TL_INC $$GSI_INC 
DEPENDPATH += $$DB_INC $$TL_INC $$GSI_INC 
LIBS += -L$$DESTDIR/.. -lklayout_db -lklayout_tl -lklayout_gsi

