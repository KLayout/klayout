
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_rba

include($$PWD/../../lib.pri)

DEFINES += MAKE_RBA_LIBRARY

SOURCES = rba.cc \
    rbaConvert.cc \
    rbaInspector.cc \
    rbaUtils.cc \
    rbaInternal.cc \
    rbaMarshal.cc

HEADERS += \
    rbaConvert.h \
    rbaInspector.h \
    rbaUtils.h \
    rba.h \
    rbaMarshal.h \
    rbaInternal.h \
    rbaCommon.h

# NOTE: ../common needs to be before RUBYINCLUDE since there is a config.h too.
INCLUDEPATH += ../common "$$RUBYINCLUDE" "$$RUBYINCLUDE2" $$TL_INC $$GSI_INC
DEPENDPATH += ../common "$$RUBYINCLUDE" "$$RUBYINCLUDE2" $$TL_INC $$GSI_INC
LIBS += "$$RUBYLIBFILE" -L$$DESTDIR -lklayout_tl -lklayout_gsi
  
