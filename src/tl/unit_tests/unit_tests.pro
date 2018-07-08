
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
  tlInt128Support.cc \
  tlXMLParser.cc \

equals(HAVE_QT, "0") {
  # nothing
} else {
  SOURCES += \
    tlWebDAV.cc \
    tlDeferredExecution.cc \
    tlFileSystemWatcher.cc \
}

equals(HAVE_CURL, "1") {
  SOURCES += \
    tlHttpStream.cc \
} else {
  equals(HAVE_QT, "0") {
    # no HTTP stream available
  } else {
    SOURCES += \
      tlHttpStream.cc \
  }
}



INCLUDEPATH += $$TL_INC
DEPENDPATH += $$TL_INC

LIBS += -L$$DESTDIR_UT -lklayout_tl

