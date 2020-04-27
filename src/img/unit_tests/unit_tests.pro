
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = img_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  imgObject.cc \
    imgFile.cc

INCLUDEPATH += $$IMG_INC $$DB_INC $$TL_INC $$LAYBASIC_INC $$GSI_INC
DEPENDPATH += $$IMG_INC $$DB_INC $$TL_INC $$LAYBASIC_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_img -lklayout_laybasic -lklayout_db -lklayout_tl -lklayout_gsi

