
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_pex

include($$PWD/../../lib.pri)

DEFINES += MAKE_PEX_LIBRARY

SOURCES = \
  gsiDeclRNetExtractor.cc \
  gsiDeclRNetwork.cc \
  pexForceLink.cc \
  pexRExtractor.cc \
  gsiDeclRExtractor.cc \
  pexRExtractorTech.cc \
  pexRNetExtractor.cc \
  pexRNetwork.cc \
  pexSquareCountingRExtractor.cc \
  pexTriangulationRExtractor.cc

HEADERS = \
  pexForceLink.h \
  pexRExtractor.h \
  pexRExtractorTech.h \
  pexRNetExtractor.h \
  pexRNetwork.h \
  pexSquareCountingRExtractor.h \
  pexTriangulationRExtractor.h

RESOURCES = \

INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db

