
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_img

include($$PWD/../../lib.pri)

DEFINES += MAKE_IMG_LIBRARY

HEADERS = \
  imgLandmarksDialog.h \
  imgNavigator.h \
  imgObject.h \
  imgPlugin.h \
  imgPropertiesPage.h \
  imgService.h \
  imgWidgets.h \
  imgForceLink.h \
  imgCommon.h

FORMS = \
  AddNewImageDialog.ui \
  ImageLandmarksDialog.ui \
  ImagePropertiesPage.ui \

SOURCES = \
  gsiDeclImg.cc \
  imgLandmarksDialog.cc \
  imgNavigator.cc \
  imgObject.cc \
  imgPlugin.cc \
  imgPropertiesPage.cc \
  imgService.cc \
  imgWidgets.cc \
  imgForceLink.cc

INCLUDEPATH += $$TL_INC $$DB_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC
DEPENDPATH += $$TL_INC $$DB_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi -lklayout_laybasic -lklayout_db

