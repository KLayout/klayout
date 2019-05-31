
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_tl

include($$PWD/../../lib.pri)

DEFINES += MAKE_TL_LIBRARY

FORMS =

SOURCES = \
    tlAssert.cc \
    tlClassRegistry.cc \
    tlDataMapping.cc \
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
    tlTimer.cc \
    tlVariant.cc \
    tlFileUtils.cc \
    tlArch.cc \
    tlCommandLineParser.cc \
    tlUnitTest.cc \
    tlInt128Support.cc \
    tlXMLParser.cc \
    tlXMLWriter.cc \
    tlThreadedWorkers.cc \
    tlThreads.cc \
    tlDeferredExecution.cc \
    tlUri.cc \
    tlLongInt.cc \
    tlUniqueId.cc \
    tlList.cc \
    tlEquivalenceClusters.cc \
    tlUniqueName.cc

HEADERS = \
    tlAlgorithm.h \
    tlAssert.h \
    tlClassRegistry.h \
    tlDataMapping.h \
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
    tlTimer.h \
    tlTypeTraits.h \
    tlUtils.h \
    tlVariant.h \
    tlVariantUserClasses.h \
    tlVector.h \
    tlCommon.h \
    tlMath.h \
    tlCpp.h \
    tlFileUtils.h \
    tlArch.h \
    tlCommandLineParser.h \
    tlUnitTest.h \
    tlInt128Support.h \
    tlDefs.h \
    tlXMLParser.h \
    tlXMLWriter.h \
    tlThreadedWorkers.h \
    tlThreads.h \
    tlDeferredExecution.h \
    tlUri.h \
    tlLongInt.h \
    tlUniqueId.h \
    tlList.h \
    tlEquivalenceClusters.h \
    tlUniqueName.h

equals(HAVE_CURL, "1") {

  HEADERS += \
    tlWebDAV.h \

  SOURCES += \
    tlHttpStreamCurl.cc \
    tlWebDAV.cc \

} else {

  !equals(HAVE_QT, "0") {

    HEADERS += \
      tlHttpStreamQt.h \
      tlWebDAV.h \

    SOURCES += \
      tlHttpStreamQt.cc \
      tlWebDAV.cc \

  } else {

    SOURCES += \
      tlHttpStreamNoQt.cc \

  }

}

!equals(HAVE_QT, "0") {

  HEADERS += \
    tlDeferredExecutionQt.h \
    tlFileSystemWatcher.h \

  SOURCES += \
    tlDeferredExecutionQt.cc \
    tlFileSystemWatcher.cc \

}

