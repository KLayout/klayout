
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = pex_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  pexRExtractorTests.cc \
  pexRNetExtractorTests.cc \
  pexSquareCountingRExtractorTests.cc \
  pexTriangulationRExtractorTests.cc

INCLUDEPATH += $$TL_INC $$DB_INC $$GSI_INC $$PEX_INC
DEPENDPATH += $$TL_INC $$DB_INC $$GSI_INC $$PEX_INC

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_pex

