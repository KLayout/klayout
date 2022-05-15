
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_img

include($$PWD/../../lib.pri)

DEFINES += MAKE_IMG_LIBRARY

!equals(HAVE_QT, "0") {

FORMS = \
  AddNewImageDialog.ui \
  ImageLandmarksDialog.ui \
  ImagePropertiesPage.ui \

}

# Disabled without Qt:

HEADERS = \
  imgLandmarksDialog.h \
  imgNavigator.h \
  imgPropertiesPage.h \
  imgWidgets.h \

SOURCES = \
  imgLandmarksDialog.cc \
  imgNavigator.cc \
  imgPropertiesPage.cc \
  imgWidgets.cc \

# Enabled without Qt:

HEADERS += \
  imgObject.h \
  imgPlugin.h \
  imgService.h \
  imgForceLink.h \
  imgCommon.h \
  imgStream.h

SOURCES += \
  gsiDeclImg.cc \
  imgObject.cc \
  imgPlugin.cc \
  imgService.cc \
  imgForceLink.cc \
  imgStream.cc

INCLUDEPATH += $$TL_INC $$DB_INC $$GSI_INC $$LAYBASIC_INC $$LAYVIEW_INC $$DB_INC
DEPENDPATH += $$TL_INC $$DB_INC $$GSI_INC $$LAYBASIC_INC $$LAYVIEW_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_db -lklayout_gsi -lklayout_laybasic -lklayout_layview -lklayout_db

!equals(HAVE_QT, "0") {

  INCLUDEPATH += $$LAYUI_INC
  DEPENDPATH += $$LAYUI_INC

  LIBS += -lklayout_layui

}
