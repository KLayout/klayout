
LIBDIR = $$OUT_PWD/../..
DESTDIR = $$LIBDIR/pymod
TARGET = lay

include($$PWD/../../lib.pri)

SOURCES = \
  layMain.cc \

HEADERS += \

INCLUDEPATH += $$PYTHONINCLUDE $$TL_INC $$GSI_INC $$PYA_INC
DEPENDPATH += $$PYTHONINCLUDE $$TL_INC $$GSI_INC $$PYA_INC
LIBS += $$PYTHONLIBFILE -L$$LIBDIR -lklayout_tl -lklayout_db -lklayout_gsi -lklayout_pya -lklayout_lay -lklayout_laybasic

# Python is somewhat sloppy and relies on the compiler initializing fields 
# of strucs to 0:
QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-missing-field-initializers

