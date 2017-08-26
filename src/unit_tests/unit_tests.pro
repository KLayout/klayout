
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)
include($$PWD/../with_all_libs.pri)

TEMPLATE = app

TARGET = ut_runner

SOURCES = \
  imgObject.cc \
  layAnnotationShapes.cc \
  layBitmap.cc \
  layBitmapsToImage.cc \
  layLayerProperties.cc \
  layParsedLayerSource.cc \
  layRenderer.cc \
  laySalt.cc \
  laySnap.cc \
  pya.cc \
  rba.cc \
  rdb.cc \

# main components:
SOURCES += \
  unit_test_main.cc \

equals(HAVE_QT5, "1") {
  QT += testlib
} else {
  CONFIG += qtestlib
}

# TODO: remove later
INCLUDEPATH += $$GSI_TEST_INC
DEPENDPATH += $$GSI_TEST_INC
LIBS += -L$$DESTDIR -lgsi_test

