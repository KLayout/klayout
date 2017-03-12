
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)
include($$PWD/../unit_tests.pri)

TEMPLATE = app

TARGET = ut_runner

HEADERS = \
  gsiTest.h

SOURCES = \
  dbArray.cc \
  dbBox.cc \
  dbBoxScanner.cc \
  dbBoxTree.cc \
  dbCell.cc \
  dbCellGraphUtils.cc \
  dbCellHullGenerator.cc \
  dbCellMapping.cc \
  dbCIFReader.cc \
  dbClip.cc \
  dbDXFReader.cc \
  dbEdge.cc \
  dbEdgePair.cc \
  dbEdgePairRelations.cc \
  dbEdgePairs.cc \
  dbEdgeProcessor.cc \
  dbEdges.cc \
  dbEdgesToContours.cc \
  dbGDS2Reader.cc \
  dbGDS2Writer.cc \
  dbLayer.cc \
  dbLayerMapping.cc \
  dbLayout.cc \
  dbLayoutDiff.cc \
  dbLayoutQuery.cc \
  dbLibraries.cc \
  dbMatrix.cc \
  dbOASISReader.cc \
  dbOASISWriter.cc \
  dbOASISWriter2.cc \
  dbObject.cc \
  dbPath.cc \
  dbPCells.cc \
  dbPoint.cc \
  dbPolygon.cc \
  dbPolygonTools.cc \
  dbPropertiesRepository.cc \
  dbRecursiveShapeIterator.cc \
  dbRegion.cc \
  dbShape.cc \
  dbShapeArray.cc \
  dbShapeRepository.cc \
  dbShapes.cc \
  dbStreamLayers.cc \
  dbText.cc \
  dbTilingProcessor.cc \
  dbTrans.cc \
  dbVector.cc \
  dbWriterTools.cc \
  extGerberImport.cc \
  extLEFDEFImport.cc \
  extNetTracer.cc \
  gsiExpression.cc \
  imgObject.cc \
  layAnnotationShapes.cc \
  layBitmap.cc \
  layBitmapsToImage.cc \
  layLayerProperties.cc \
  layParsedLayerSource.cc \
  layRenderer.cc \
  pya.cc \
  rba.cc \
  rdb.cc \
  tlAlgorithm.cc \
  tlClassRegistry.cc \
  tlDataMapping.cc \
  tlDeferredExecution.cc \
  tlDeflate.cc \
  tlExpression.cc \
  tlEvents.cc \
  tlGlobPattern.cc \
  tlIntervalMap.cc \
  tlIntervalSet.cc \
  tlKDTree.cc \
  tlObject.cc \
  tlReuseVector.cc \
  tlStableVector.cc \
  tlString.cc \
  tlThreadedWorkers.cc \
  tlUtils.cc \
  tlVariant.cc \
  tlXMLParser.cc \
  gsiTest.cc \
    tlFileSystemWatcher.cc \
    laySaltGrain.cc

# main components:
SOURCES += \
  unit_test_main.cc \

equals(HAVE_QT5, "1") {
  QT += testlib
} else {
  CONFIG += qtestlib
}

