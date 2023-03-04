
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_pya

include($$PWD/../../lib.pri)

DEFINES += MAKE_PYA_LIBRARY

SOURCES = \
  pya.cc \
  pyaConvert.cc \
  pyaHelpers.cc \
  pyaInspector.cc \
  pyaInternal.cc \
  pyaCallables.cc \
  pyaMarshal.cc \
  pyaObject.cc \
  pyaRefs.cc \
  pyaUtils.cc \
  pyaModule.cc \
  pyaSignalHandler.cc \
  pyaStatusChangedListener.cc \
  gsiDeclPya.cc

HEADERS += \
  pya.h \
  pyaCommon.h \
  pyaConvert.h \
  pyaHelpers.h \
  pyaInspector.h \
  pyaInternal.h \
  pyaCallables.h \
  pyaMarshal.h \
  pyaObject.h \
  pyaRefs.h \
  pyaUtils.h \
  pyaModule.h \
  pyaSignalHandler.h \
  pyaStatusChangedListener.h

INCLUDEPATH += "$$PYTHONINCLUDE" $$VERSION_INC $$TL_INC $$GSI_INC
DEPENDPATH += "$$PYTHONINCLUDE" $$TL_INC $$GSI_INC
LIBS += "$$PYTHONLIBFILE" -L$$DESTDIR -lklayout_tl -lklayout_gsi

!msvc {
  # Python is somewhat sloppy and relies on the compiler initializing fields
  # of strucs to 0:
  QMAKE_CXXFLAGS_WARN_ON += \
      -Wno-missing-field-initializers
}

msvc {
  # pyconfig.h tries to tell us which library to link, but we know better ..
  QMAKE_LINK_FLAGS += /NODEFAULTLIB:python*
}
