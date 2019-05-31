
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = tl_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  tlAlgorithm.cc \
  tlClassRegistry.cc \
  tlCommandLineParser.cc \
  tlDataMapping.cc \
  tlDeflate.cc \
  tlEvents.cc \
  tlExpression.cc \
  tlFileUtils.cc \
  tlGlobPattern.cc \
  tlIntervalMap.cc \
  tlIntervalSet.cc \
  tlKDTree.cc \
  tlMath.cc \
  tlObject.cc \
  tlReuseVector.cc \
  tlStableVector.cc \
  tlString.cc \
  tlThreadedWorkers.cc \
  tlThreads.cc \
  tlUtils.cc \
  tlVariant.cc \
  tlXMLParser.cc \
  tlUri.cc \
  tlStreamTests.cc \
  tlWebDAV.cc \
  tlHttpStream.cc \
  tlInt128Support.cc \
  tlLongInt.cc \
  tlUniqueIdTests.cc \
  tlListTests.cc \
  tlEquivalenceClustersTests.cc \
    tlUniqueNameTests.cc

!equals(HAVE_QT, "0") {

  SOURCES += \
    tlDeferredExecution.cc \
    tlFileSystemWatcher.cc \

}

INCLUDEPATH += $$TL_INC
DEPENDPATH += $$TL_INC

LIBS += -L$$DESTDIR_UT -lklayout_tl

