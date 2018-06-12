
include($$PWD/../klayout.pri)

TEMPLATE = lib

INCLUDEPATH += $$DB_INC $$TL_INC $$GSI_INC $$LAYBASIC_INC $$LAY_INC
DEPENDPATH += $$DB_INC $$TL_INC $$GSI_INC $$LAYBASIC_INC $$LAY_INC
LIBS += -L$$DESTDIR/.. -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_lay
