
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_pex

include($$PWD/../../lib.pri)

DEFINES += MAKE_PEX_LIBRARY

SOURCES = \
  pexForceLink.cc \
  pexRExtractor.cc \
  gsiDeclRExtractor.cc \
  pexSquareCountingRExtractor.cc \
  pexTriangulationRExtractor.cc

HEADERS = \
  pexForceLink.h \
  pexRExtractor.h \
  pexSquareCountingRExtractor.h \
  pexTriangulationRExtractor.h

RESOURCES = \

INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db

