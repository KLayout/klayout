
DESTDIR_KLP = $$OUT_PWD/../../..
DESTDIR = $$OUT_PWD/..

TEMPLATE = lib

INCLUDEPATH += ../../../db ../../../tl ../../../gsi ../../../laybasic ../../../lay ../../../common
DEPENDPATH += ../../../db ../../../tl ../../../gsi ../../../laybasic ../../../lay ../../../common
LIBS += -L$$DESTDIR_KLP -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_lay

QMAKE_POST_LINK += $(COPY) $(DESTDIR)$(TARGET) $$DESTDIR_KLP/$${TARGET}.klp
