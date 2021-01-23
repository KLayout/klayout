
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = tl_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  tlAlgorithmTests.cc \
  tlClassRegistryTests.cc \
  tlCommandLineParserTests.cc \
  tlCopyOnWriteTests.cc \
  tlDataMappingTests.cc \
  tlDeferredExecutionTests.cc \
  tlDeflateTests.cc \
  tlEventsTests.cc \
  tlExpressionTests.cc \
  tlFileSystemWatcherTests.cc \
  tlFileUtilsTests.cc \
  tlHttpStreamTests.cc \
  tlIncludeTests.cc \
  tlInt128SupportTests.cc \
  tlIntervalMapTests.cc \
  tlIntervalSetTests.cc \
  tlKDTreeTests.cc \
  tlLongIntTests.cc \
  tlMathTests.cc \
  tlObjectTests.cc \
  tlReuseVectorTests.cc \
  tlStableVectorTests.cc \
  tlStreamTests.cc \
  tlStringTests.cc \
  tlThreadedWorkersTests.cc \
  tlThreadsTests.cc \
  tlUniqueIdTests.cc \
  tlListTests.cc \
  tlEquivalenceClustersTests.cc \
    tlUniqueNameTests.cc \
    tlGlobPatternTests.cc \
    tlRecipeTests.cc \
    tlUriTests.cc \
  tlUtilsTests.cc \
  tlVariantTests.cc \
  tlWebDAVTests.cc \
  tlXMLParserTests.cc

!equals(HAVE_QT, "0") {

  SOURCES += \

}

INCLUDEPATH += $$TL_INC
DEPENDPATH += $$TL_INC

LIBS += -L$$DESTDIR_UT -lklayout_tl

