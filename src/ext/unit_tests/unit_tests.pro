
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = ext_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  extNetTracer.cc \

INCLUDEPATH += $$EXT_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$EXT_INC $$TL_INC $$LAYBASIC_INC $$DB_INC $$GSI_INC

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR_UT/ext/ext $$DESTDIR_UT/laybasic/laybasic
DEPENDPATH += $$DESTDIR_UT/ext/ext $$DESTDIR_UT/laybasic/laybasic

LIBS += -L$$DESTDIR_UT -lklayout_ext -lklayout_laybasic -lklayout_db -lklayout_tl -lklayout_gsi

# TODO: ideally this should not be there:
INCLUDEPATH += $$DESTDIR_UT/lay/lay
DEPENDPATH += $$DESTDIR_UT/lay/lay
LIBS += -L$$DESTDIR_UT -lklayout_lay

