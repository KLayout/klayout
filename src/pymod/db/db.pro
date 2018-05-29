
LIBDIR = $$OUT_PWD/../..
DESTDIR = $$LIBDIR/pymod
TARGET = db

include($$PWD/../../lib.pri)

SOURCES = \
  dbMain.cc \

HEADERS += \

INCLUDEPATH += $$PYTHONINCLUDE $$TL_INC $$GSI_INC $$DB_INC $$GSI_INC $$PYA_INC
DEPENDPATH += $$PYTHONINCLUDE $$TL_INC $$GSI_INC $$DB_INC $$GSI_INC $$PYA_INC
LIBS += $$PYTHONLIBFILE -L$$LIBDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_gsi -lklayout_pya

# Python is somewhat sloppy and relies on the compiler initializing fields 
# of strucs to 0:
QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-missing-field-initializers

