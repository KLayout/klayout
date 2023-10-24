
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_tl

include($$PWD/../../lib.pri)

DEFINES += MAKE_TL_LIBRARY

FORMS =

SOURCES = \
    tlAssert.cc \
    tlBase64.cc \
    tlColor.cc \
    tlClassRegistry.cc \
    tlCopyOnWrite.cc \
    tlDataMapping.cc \
    tlDeflate.cc \
    tlException.cc \
    tlExceptions.cc \
    tlExpression.cc \
    tlEvents.cc \
    tlGlobPattern.cc \
    tlHeap.cc \
    tlHttpStream.cc \
    tlInclude.cc \
    tlInternational.cc \
    tlLog.cc \
    tlObject.cc \
    tlProgress.cc \
    tlPixelBuffer.cc \
    tlResources.cc \
    tlScriptError.cc \
    tlSleep.cc \
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
    tlSList.cc \
    tlEquivalenceClusters.cc \
    tlUniqueName.cc \
    tlRecipe.cc \
    tlEnv.cc

HEADERS = \
    tlAlgorithm.h \
    tlAssert.h \
    tlBase64.h \
    tlColor.h \
    tlClassRegistry.h \
    tlCopyOnWrite.h \
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
    tlInclude.h \
    tlInternational.h \
    tlIntervalMap.h \
    tlIntervalSet.h \
    tlKDTree.h \
    tlLog.h \
    tlObject.h \
    tlObjectCollection.h \
    tlProgress.h \
    tlPixelBuffer.h \
    tlResources.h \
    tlReuseVector.h \
    tlScriptError.h \
    tlSleep.h \
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
    tlSList.h \
    tlEquivalenceClusters.h \
    tlUniqueName.h \
    tlRecipe.h \
    tlSelect.h \
    tlEnv.h 

equals(HAVE_GIT2, "1") {

  HEADERS += \
    tlGit.h

  SOURCES += \
    tlGit.cc

}

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

