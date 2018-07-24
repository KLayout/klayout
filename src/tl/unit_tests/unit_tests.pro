
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = tl_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  tlAlgorithm.cc \
  tlClassRegistry.cc \
  tlCommandLineParser.cc \
  tlDataMapping.cc \
  tlDeferredExecution.cc \
  tlDeflate.cc \
  tlEvents.cc \
  tlExpression.cc \
  tlFileSystemWatcher.cc \
  tlFileUtils.cc \
  tlGlobPattern.cc \
  tlHttpStream.cc \
  tlIntervalMap.cc \
  tlIntervalSet.cc \
  tlKDTree.cc \
  tlMath.cc \
  tlObject.cc \
  tlReuseVector.cc \
  tlStableVector.cc \
  tlString.cc \
  tlThreadedWorkers.cc \
  tlUtils.cc \
  tlVariant.cc \
  tlWebDAV.cc \
  tlXMLParser.cc \
    tlInt128Support.cc \
    tlLongInt.cc

INCLUDEPATH += $$TL_INC
DEPENDPATH += $$TL_INC

LIBS += -L$$DESTDIR_UT -lklayout_tl

