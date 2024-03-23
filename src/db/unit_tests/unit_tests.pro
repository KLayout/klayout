
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = db_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  dbCompoundOperationTests.cc \
  dbFillToolTests.cc \
  dbLogTests.cc \
  dbRecursiveInstanceIteratorTests.cc \
  dbRegionCheckUtilsTests.cc \
  dbTriangleTests.cc \
  dbTrianglesTests.cc \
  dbUtilsTests.cc \
  dbWriterTools.cc \
  dbLoadLayoutOptionsTests.cc \
  dbSaveLayoutOptionsTests.cc \
  dbHierarchyBuilderTests.cc \
  dbRecursiveShapeIteratorTests.cc \
  dbHierProcessorTests.cc \
  dbDeepRegionTests.cc \
  dbDeepShapeStoreTests.cc \
  dbHierNetworkProcessorTests.cc \
  dbNetlistTests.cc \
  dbNetlistExtractorTests.cc \
  dbNetlistDeviceExtractorTests.cc \
  dbNetlistDeviceClassesTests.cc \
  dbLayoutToNetlistTests.cc \
  dbLayoutToNetlistWriterTests.cc \
  dbLayoutToNetlistReaderTests.cc \
  dbNetlistWriterTests.cc \
  dbCellVariantsTests.cc \
  dbDeepEdgesTests.cc \
  dbDeepEdgePairsTests.cc \
  dbNetlistCompareTests.cc \
  dbNetlistReaderTests.cc \
  dbLayoutVsSchematicTests.cc \
  dbLayoutQueryTests.cc \
  dbPolygonToolsTests.cc \
  dbTechnologyTests.cc \
  dbStreamLayerTests.cc \
  dbVectorTests.cc \
  dbVariableWidthPathTests.cc \
  dbTransTests.cc \
  dbTilingProcessorTests.cc \
  dbTextsTests.cc \
  dbTextTests.cc \
  dbShapesTests.cc \
  dbShapeRepositoryTests.cc \
  dbShapeArrayTests.cc \
  dbShapeTests.cc \
  dbRegionTests.cc \
  dbPropertiesRepositoryTests.cc \
  dbPolygonTests.cc \
  dbPointTests.cc \
  dbPCellsTests.cc \
  dbPathTests.cc \
  dbObjectTests.cc \
  dbMatrixTests.cc \
  dbLibrariesTests.cc \
  dbLayoutUtilsTests.cc \
  dbLayoutDiffTests.cc \
  dbLayoutTests.cc \
  dbLayerMappingTests.cc \
  dbLayerTests.cc \
  dbExpressionTests.cc \
  dbEdgesToContoursTests.cc \
  dbEdgesTests.cc \
  dbEdgeProcessorTests.cc \
  dbEdgePairsTests.cc \
  dbEdgePairRelationsTests.cc \
  dbEdgePairTests.cc \
  dbEdgeTests.cc \
  dbEdgesUtilsTests.cc \
  dbClipTests.cc \
  dbCellMappingTests.cc \
  dbCellHullGeneratorTests.cc \
  dbCellGraphUtilsTests.cc \
  dbCellTests.cc \
  dbBoxTreeTests.cc \
  dbBoxScannerTests.cc \
  dbBoxTests.cc \
  dbArrayTests.cc \
  dbDeepTextsTests.cc \
  dbNetShapeTests.cc \
  dbHierNetsProcessorTests.cc \
  dbRegionProcessorTests.cc \
  dbAsIfFlatRegionTests.cc

INCLUDEPATH += $$TL_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi

