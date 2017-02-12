
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

DEFINES += MAKE_GSI_LIBRARY

TEMPLATE = lib

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
  gsiObject.cc \
  gsiSerialisation.cc \
  gsiTypes.cc \
  gsiObjectHolder.cc \

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
    gsiCommon.h

# Note: unlike other modules, the tl declarations have to go here
# since gsi is dependent on tl
SOURCES += gsiDeclTl.cc 

INCLUDEPATH += ../tl
DEPENDPATH += ../tl
LIBS += -L$$DESTDIR -ltl

