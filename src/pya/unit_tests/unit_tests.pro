
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = pya_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  pyaTests.cc

INCLUDEPATH += $$PYA_INC $$DB_INC $$TL_INC $$GSI_INC
DEPENDPATH += $$PYA_INC $$DB_INC $$TL_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_pya -lklayout_db -lklayout_tl -lklayout_gsi

