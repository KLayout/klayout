
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_gsi

include($$PWD/../../lib.pri)

DEFINES += MAKE_GSI_LIBRARY

SOURCES = \
  gsi.cc \
  gsiClassBase.cc \
  gsiClass.cc \
  gsiDeclBasic.cc \
  gsiDeclInternal.cc \
  gsiExpression.cc \
  gsiExternalMain.cc \
  gsiInterpreter.cc \
  gsiMethods.cc \
  gsiInspector.cc \
  gsiObject.cc \
  gsiSerialisation.cc \
  gsiTypes.cc \
  gsiSignals.cc \
  gsiObjectHolder.cc \
  gsiVariantArgs.cc

HEADERS = \
  gsiCallback.h \
  gsiCallbackVar.h \
  gsiClassBase.h \
  gsiClass.h \
  gsiDeclBasic.h \
  gsiDecl.h \
  gsiExpression.h \
  gsiExternalMain.h \
  gsi.h \
  gsiInspector.h \
  gsiInterpreter.h \
  gsiIterators.h \
  gsiMethods.h \
  gsiMethodsVar.h \
  gsiObject.h \
  gsiSerialisation.h \
  gsiSignals.h \
  gsiTypes.h \
  gsiObjectHolder.h \
  gsiCommon.h \
  gsiVariantArgs.h

# Note: unlike other modules, the tl declarations have to go here
# since gsi is dependent on tl
SOURCES += \
  gsiDeclTl.cc \
  gsiDeclTlPixelBuffer.cc 

INCLUDEPATH += $$TL_INC
DEPENDPATH += $$TL_INC
LIBS += -L$$DESTDIR -lklayout_tl

