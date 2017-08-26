
DESTDIR = $$OUT_PWD/..
TARGET = klayout_pya

include($$PWD/../lib.pri)

DEFINES += MAKE_PYA_LIBRARY

SOURCES = \
  pya.cc \
  pyaConvert.cc \
  pyaHelpers.cc \
  pyaInspector.cc \
  pyaMarshal.cc \
  pyaObject.cc \
  pyaRefs.cc \
  pyaUtils.cc \

INCLUDEPATH += $$PYTHONINCLUDE $$TL_INC $$GSI_INC
DEPENDPATH += $$PYTHONINCLUDE $$TL_INC $$GSI_INC
LIBS += $$PYTHONLIBFILE -L$$DESTDIR -lklayout_tl -lklayout_gsi

# Python is somewhat sloppy and relies on the compiler initializing fields 
# of strucs to 0:
QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-missing-field-initializers

HEADERS += \
    pya.h \
    pyaCommon.h \
    pyaConvert.h \
    pyaHelpers.h \
    pyaInspector.h \
    pyaMarshal.h \
    pyaObject.h \
    pyaRefs.h \
    pyaUtils.h

