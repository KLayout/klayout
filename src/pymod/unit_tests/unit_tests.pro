
DESTDIR_UT = $$OUT_PWD/../..

TARGET = pymod_tests

include($$PWD/../../klayout.pri)
include($$PWD/../../lib_ut.pri)

SOURCES = \
  pymod_tests.cc

DEFINES += \
  PYTHON=$$PYTHON \
  PYTHONPATH=$$DESTDIR_UT/pymod


INCLUDEPATH += $$DB_INC $$TL_INC $$GSI_INC
DEPENDPATH += $$DB_INC $$TL_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi
