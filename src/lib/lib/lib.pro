
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_lib

include($$PWD/../../lib.pri)

DEFINES += MAKE_LIB_LIBRARY

HEADERS = \
  libBasicArc.h \
  libBasicCircle.h \
  libBasicDonut.h \
  libBasicEllipse.h \
  libBasicPie.h \
  libBasicRoundPath.h \
  libBasicRoundPolygon.h \
  libBasicStrokedPolygon.h \
  libBasicText.h \
  libForceLink.h

SOURCES = \
  libForceLink.cc \
  libBasic.cc \
  libBasicArc.cc \
  libBasicCircle.cc \
  libBasicDonut.cc \
  libBasicEllipse.cc \
  libBasicPie.cc \
  libBasicRoundPath.cc \
  libBasicRoundPolygon.cc \
  libBasicStrokedPolygon.cc \
  libBasicText.cc

!equals(HAVE_QT, "0") {
  RESOURCES = \
    libResources.qrc
}

INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_gsi -lklayout_tl -lklayout_db

