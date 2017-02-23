
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

DEFINES += MAKE_LAYBASIC_LIBRARY

TEMPLATE = lib

FORMS = \
  AlignCellOptionsDialog.ui \
  BookmarkManagementForm.ui \
  BrowseInstancesConfigPage.ui \
  BrowseInstancesForm.ui \
  BrowserDialog.ui \
  BrowserPanel.ui \
  BrowseShapesConfigPage.ui \
  BrowseShapesForm.ui \
  CIFReaderOptionPage.ui \
  CIFWriterOptionPage.ui \
  CellSelectionForm.ui \
  ClearLayerModeDialog.ui \
  ConfigurationDialog.ui \
  CopyCellModeDialog.ui \
  DXFReaderOptionPage.ui \
  DXFWriterOptionPage.ui \
  DeleteCellModeDialog.ui \
  DuplicateLayerDialog.ui \
  EditStipplesForm.ui \
  FlattenInstOptionsDialog.ui \
  GDS2ReaderOptionPage.ui \
  GDS2WriterOptionPage.ui \
  GridNetConfigPage.ui \
  LayerMappingWidget.ui \
  LayerSourceDialog.ui \
  LayoutProperties.ui \
  LayoutViewConfigPage1.ui \
  LayoutViewConfigPage2a.ui \
  LayoutViewConfigPage2b.ui \
  LayoutViewConfigPage2c.ui \
  LayoutViewConfigPage3a.ui \
  LayoutViewConfigPage3b.ui \
  LayoutViewConfigPage3c.ui \
  LayoutViewConfigPage3f.ui \
  LayoutViewConfigPage4.ui \
  LayoutViewConfigPage5.ui \
  LayoutViewConfigPage6.ui \
  LayoutViewConfigPage7.ui \
  LayoutViewConfigPage.ui \
  LibraryCellSelectionForm.ui \
  LoadLayoutOptionsDialog.ui \
  MarkerBrowserConfigPage2.ui \
  MarkerBrowserConfigPage.ui \
  MarkerBrowserDialog.ui \
  MarkerBrowserPage.ui \
  MarkerBrowserSnapshotView.ui \
  MoveOptionsDialog.ui \
  MoveToOptionsDialog.ui \
  NewCellPropertiesDialog.ui \
  NewLayerPropertiesDialog.ui \
  NewLayoutPropertiesDialog.ui \
  OASISWriterOptionPage.ui \
  OpenLayoutModeDialog.ui \
  PropertiesDialog.ui \
  RenameCellDialog.ui \
  ReplaceCellOptionsDialog.ui \
  SaveLayoutOptionsDialog.ui \
  SaveLayoutAsOptionsDialog.ui \
  SelectStippleForm.ui \
  SimpleCellSelectionForm.ui \
  TipDialog.ui \
  UserPropertiesForm.ui \
  UserPropertiesEditForm.ui \
    SpecificLoadLayoutOptionsDialog.ui \
    CommonReaderOptionsPage.ui \
    SelectLineStyleForm.ui \
    LayoutViewConfigPage6a.ui \
    EditLineStylesForm.ui

RESOURCES = \

SOURCES = \
  gtf.cc \
  gsiDeclLayDialogs.cc \
  gsiDeclLayLayers.cc \
  gsiDeclLayLayoutView.cc \
  gsiDeclLayMarker.cc \
  gsiDeclLayMenu.cc \
  gsiDeclLayPlugin.cc \
  gsiDeclLayStream.cc \
  layAbstractMenu.cc \
  layAbstractMenuProvider.cc \
  layAnnotationShapes.cc \
  layBitmap.cc \
  layBitmapRenderer.cc \
  layBitmapsToImage.cc \
  layBookmarkList.cc \
  layBookmarkManagementForm.cc \
  layBrowseInstancesForm.cc \
  layBrowser.cc \
  layBrowserDialog.cc \
  layBrowserPanel.cc \
  layBrowseShapesForm.cc \
  layCanvasPlane.cc \
  layCellSelectionForm.cc \
  layCellTreeModel.cc \
  layCellView.cc \
  layCIFReaderPlugin.cc \
  layCIFWriterPlugin.cc \
  layColorPalette.cc \
  layConfigurationDialog.cc \
  layConverters.cc \
  layCursor.cc \
  layDialogs.cc \
  layDisplayState.cc \
  layDitherPattern.cc \
  layDrawing.cc \
  layDXFReaderPlugin.cc \
  layDXFWriterPlugin.cc \
  layEditable.cc \
  layEditStipplesForm.cc \
  layEditStippleWidget.cc \
  layFileDialog.cc \
  layFinder.cc \
  layFixedFont.cc \
  layGDS2ReaderPlugin.cc \
  layGDS2WriterPlugin.cc \
  layGridNet.cc \
  layHierarchyControlPanel.cc \
  layLayerControlPanel.cc \
  layLayerMappingWidget.cc \
  layLayerProperties.cc \
  layLayerToolbox.cc \
  layLayerTreeModel.cc \
  layLayoutCanvas.cc \
  layLayoutPropertiesForm.cc \
  layLayoutView.cc \
  layLayoutViewConfigPages.cc \
  layLoadLayoutOptionsDialog.cc \
  layMarker.cc \
  layMouseTracker.cc \
  layMove.cc \
  layOASISWriterPlugin.cc \
  layObjectInstPath.cc \
  layParsedLayerSource.cc \
  layPlugin.cc \
  layProperties.cc \
  layPropertiesDialog.cc \
  layQtTools.cc \
  layRedrawLayerInfo.cc \
  layRedrawThreadCanvas.cc \
  layRedrawThread.cc \
  layRedrawThreadWorker.cc \
  layRenderer.cc \
  layRubberBox.cc \
  laySaveLayoutOptionsDialog.cc \
  laySelector.cc \
  laySelectStippleForm.cc \
  laySnap.cc \
  layStipplePalette.cc \
  layStream.cc \
  layTechnology.cc \
  layTipDialog.cc \
  layViewObject.cc \
  layViewOp.cc \
  layViewport.cc \
  layWidgets.cc \
  layZoomBox.cc \
  rdbInfoWidget.cc \
  rdbMarkerBrowser.cc \
  rdbMarkerBrowserDialog.cc \
  rdbMarkerBrowserPage.cc \
    layOASISReaderPlugin.cc \
    layCommonReaderPlugin.cc \
    layLineStyles.cc \
    laySelectLineStyleForm.cc \
    layLineStylePalette.cc \
    layEditLineStylesForm.cc \
    layEditLineStyleWidget.cc

HEADERS = \
  gtf.h \
  layAbstractMenu.h \
  layAbstractMenuProvider.h \
  layAnnotationShapes.h \
  layBitmap.h \
  layBitmapRenderer.h \
  layBitmapsToImage.h \
  layBookmarkList.h \
  layBookmarkManagementForm.h \
  layBrowseInstancesForm.h \
  layBrowserDialog.h \
  layBrowser.h \
  layBrowserPanel.h \
  layBrowseShapesForm.h \
  layCanvasPlane.h \
  layCellSelectionForm.h \
  layCellTreeModel.h \
  layCellView.h \
  layCIFReaderPlugin.h \
  layCIFWriterPlugin.h \
  layColorPalette.h \
  layConfigurationDialog.h \
  layConverters.h \
  layCursor.h \
  layDialogs.h \
  layDisplayState.h \
  layDitherPattern.h \
  layDrawing.h \
  layDXFReaderPlugin.h \
  layDXFWriterPlugin.h \
  layEditable.h \
  layEditStipplesForm.h \
  layEditStippleWidget.h \
  layFileDialog.h \
  layFinder.h \
  layFixedFont.h \
  layGDS2ReaderPlugin.h \
  layGDS2WriterPlugin.h \
  layGridNet.h \
  layHierarchyControlPanel.h \
  layLayerControlPanel.h \
  layLayerMappingWidget.h \
  layLayerProperties.h \
  layLayerToolbox.h \
  layLayerTreeModel.h \
  layLayoutCanvas.h \
  layLayoutPropertiesForm.h \
  layLayoutViewConfigPages.h \
  layLayoutView.h \
  layLoadLayoutOptionsDialog.h \
  layMarker.h \
  layMouseTracker.h \
  layMove.h \
  layOASISWriterPlugin.h \
  layObjectInstPath.h \
  layParsedLayerSource.h \
  layPlugin.h \
  layPropertiesDialog.h \
  layProperties.h \
  layQtTools.h \
  layRedrawLayerInfo.h \
  layRedrawThreadCanvas.h \
  layRedrawThread.h \
  layRedrawThreadWorker.h \
  layRenderer.h \
  layRubberBox.h \
  laySaveLayoutOptionsDialog.h \
  laySelector.h \
  laySelectStippleForm.h \
  laySnap.h \
  layStipplePalette.h \
  layStream.h \
  layTechnology.h \
  layTipDialog.h \
  layViewObject.h \
  layViewOp.h \
  layViewport.h \
  layWidgets.h \
  layZoomBox.h \
  rdbInfoWidget.h \
  rdbMarkerBrowserDialog.h \
  rdbMarkerBrowser.h \
  rdbMarkerBrowserPage.h \
    layOASISReaderPlugin.h \
    layCommonReaderPlugin.h \
    layLineStyles.h \
    laySelectLineStyleForm.h \
    layLineStylePalette.h \
    layEditLineStylesForm.h \
    layEditLineStyleWidget.h \
    laybasicCommon.h \
    laybasicConfig.h

INCLUDEPATH += ../tl ../gsi ../db ../rdb
DEPENDPATH += ../tl ../gsi ../db ../rdb
LIBS += -L$$DESTDIR -ltl -lgsi -ldb -lrdb

INCLUDEPATH += ../gsiqt
DEPENDPATH += ../gsiqt

equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lgsiqt
}

