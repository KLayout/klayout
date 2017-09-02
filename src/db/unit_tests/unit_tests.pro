
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
  dbLayoutUtils.cc \
  dbLayoutQuery.cc \
  dbLibraries.cc \
  dbMatrix.cc \
  dbOASISReader.cc \
  dbOASISWriter2.cc \
  dbOASISWriter.cc \
  dbObject.cc \
  dbPath.cc \
  dbPCells.cc \
  dbPoint.cc \
  dbPolygon.cc \
  dbPolygonTools.cc \
  dbPropertiesRepository.cc \
  dbRecursiveShapeIterator.cc \
  dbRegion.cc \
  dbShapeArray.cc \
  dbShape.cc \
  dbShapeRepository.cc \
  dbShapes.cc \
  dbStreamLayers.cc \
  dbText.cc \
  dbTilingProcessor.cc \
  dbTrans.cc \
  dbVector.cc \
  dbWriterTools.cc \

INCLUDEPATH += $$TL_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi

