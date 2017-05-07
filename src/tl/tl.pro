
DESTDIR = $$OUT_PWD/..
TARGET = klayout_tl

include($$PWD/../klayout.pri)

DEFINES += MAKE_TL_LIBRARY

LIBS += -lz

TEMPLATE = lib

FORMS = \
  PasswordDialog.ui

SOURCES = \
  tlAssert.cc \
  tlClassRegistry.cc \
  tlDataMapping.cc \
  tlDeferredExecution.cc \
  tlDeflate.cc \
  tlException.cc \
  tlExceptions.cc \
  tlExpression.cc \
  tlEvents.cc \
  tlGlobPattern.cc \
  tlHeap.cc \
  tlHttpStream.cc \
  tlInternational.cc \
  tlLog.cc \
  tlObject.cc \
  tlProgress.cc \
  tlScriptError.cc \
  tlStaticObjects.cc \
  tlStream.cc \
  tlString.cc \
  tlSystemPaths.cc \
  tlThreadedWorkers.cc \
  tlTimer.cc \
  tlVariant.cc \
  tlXMLParser.cc \
  tlXMLWriter.cc \
    tlFileSystemWatcher.cc \
    tlFileUtils.cc \
    tlWebDAV.cc

HEADERS = \
  tlAlgorithm.h \
  tlAssert.h \
  tlClassRegistry.h \
  tlDataMapping.h \
  tlDeferredExecution.h \
  tlDeflate.h \
  tlException.h \
  tlExceptions.h \
  tlExpression.h \
  tlEvents.h \
  tlFixedVector.h \
  tlGlobPattern.h \
  tlHeap.h \
  tlHttpStream.h \
  tlInternational.h \
  tlIntervalMap.h \
  tlIntervalSet.h \
  tlKDTree.h \
  tlLog.h \
  tlObject.h \
  tlObjectCollection.h \
  tlProgress.h \
  tlReuseVector.h \
  tlScriptError.h \
  tlStableVector.h \
  tlStaticObjects.h \
  tlStream.h \
  tlString.h \
  tlSystemPaths.h \
  tlThreadedWorkers.h \
  tlTimer.h \
  tlTypeTraits.h \
  tlUtils.h \
  tlVariant.h \
  tlVariantUserClasses.h \
  tlVector.h \
  tlXMLParser.h \
  tlXMLWriter.h \
    tlFileSystemWatcher.h \
    tlCommon.h \
    tlMath.h \
    tlCpp.h \
    tlFileUtils.h \
    tlWebDAV.h

INCLUDEPATH =
DEPENDPATH =

