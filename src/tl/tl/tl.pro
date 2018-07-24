
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_tl

include($$PWD/../../lib.pri)

DEFINES += MAKE_TL_LIBRARY

LIBS += -lz

equals(HAVE_CURL, "1") {
  LIBS += -lcurl
  win32 {
    LIBS += -lwsock32    # required because we do "select"
  }
}

FORMS =

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
    tlThreadedWorkers.cc \
    tlTimer.cc \
    tlVariant.cc \
    tlXMLParser.cc \
    tlXMLWriter.cc \
    tlFileSystemWatcher.cc \
    tlFileUtils.cc \
    tlWebDAV.cc \
    tlArch.cc \
    tlCommandLineParser.cc \
    tlUnitTest.cc \
    tlInt128Support.cc \
    tlHttpStreamCurl.cc \
    tlHttpStreamQt.cc \
    tlLongInt.cc

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
    tlWebDAV.h \
    tlArch.h \
    tlCommandLineParser.h \
    tlUnitTest.h \
    tlInt128Support.h \
    tlHttpStreamCurl.h \
    tlHttpStreamQt.h \
    tlLongInt.h

INCLUDEPATH =
DEPENDPATH =

