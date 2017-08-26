
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = ext_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  extGerberImport.cc \
  extLEFDEFImport.cc \
  extNetTracer.cc \

INCLUDEPATH += $$EXT_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC $$UT_INC
DEPENDPATH += $$EXT_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC $$UT_INC

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/ext $$DESTDIR/laybasic
DEPENDPATH += $$DESTDIR/ext $$DESTDIR/laybasic

LIBS += -L$$DESTDIR_UT -lklayout_ext -lklayout_laybasic -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_ut

# TODO: ideally this should not be there:
INCLUDEPATH += $$DESTDIR/lay
DEPENDPATH += $$DESTDIR/lay
LIBS += -L$$DESTDIR -lklayout_lay

