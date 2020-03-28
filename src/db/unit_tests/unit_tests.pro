
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = db_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  dbArray.cc \
  dbBox.cc \
  dbBoxScanner.cc \
  dbBoxTree.cc \
  dbCell.cc \
  dbCellGraphUtils.cc \
  dbCellHullGenerator.cc \
  dbCellMapping.cc \
  dbClip.cc \
  dbExpression.cc \
  dbEdge.cc \
  dbEdgePair.cc \
  dbEdgePairRelations.cc \
  dbEdgePairs.cc \
  dbEdgeProcessor.cc \
  dbEdges.cc \
  dbEdgesToContours.cc \
  dbLayer.cc \
  dbLayerMapping.cc \
  dbLayout.cc \
  dbLayoutDiff.cc \
  dbLayoutUtils.cc \
  dbLibraries.cc \
  dbMatrix.cc \
  dbObject.cc \
  dbPath.cc \
  dbPCells.cc \
  dbPoint.cc \
  dbPolygon.cc \
  dbPropertiesRepository.cc \
  dbRegion.cc \
  dbShapeArray.cc \
  dbShape.cc \
  dbShapeRepository.cc \
  dbShapes.cc \
  dbText.cc \
  dbTilingProcessor.cc \
  dbTrans.cc \
  dbVector.cc \
  dbWriterTools.cc \
    dbVariableWidthPath.cc \
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
    dbStreamLayerTests.cc

INCLUDEPATH += $$TL_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi

