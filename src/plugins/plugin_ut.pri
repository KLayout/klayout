
DESTDIR_KLP = $$OUT_PWD/../../..

TEMPLATE = lib

INCLUDEPATH += ../src ../../../db ../../../tl ../../../gsi ../../../laybasic ../../../lay ../../../common ../../../ut
DEPENDPATH += ../src ../../../db ../../../tl ../../../gsi ../../../laybasic ../../../lay ../../../common ../../../ut
LIBS += -L$$DESTDIR_KLP -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_lay -lklayout_ut

QMAKE_POST_LINK += $(COPY) $$OUT_PWD/$${SHLIB_PREFIX}$${TARGET}.$${SHLIB_EXT} $$DESTDIR_KLP/$${TARGET}.klp_ut
