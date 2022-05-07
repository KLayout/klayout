
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_laybasic

include($$PWD/../../lib.pri)

DEFINES += MAKE_LAYBASIC_LIBRARY

!equals(HAVE_QT, "0") {

  FORMS = \
    AlignCellOptionsDialog.ui \
    BookmarkManagementForm.ui \
    BrowseInstancesConfigPage.ui \
    BrowseInstancesForm.ui \
    BrowserDialog.ui \
    BrowserPanel.ui \
    BrowseShapesConfigPage.ui \
    BrowseShapesForm.ui \
    CellSelectionForm.ui \
    ClearLayerModeDialog.ui \
    ConfigurationDialog.ui \
    CopyCellModeDialog.ui \
    DeleteCellModeDialog.ui \
    DuplicateLayerDialog.ui \
    EditStipplesForm.ui \
    FlattenInstOptionsDialog.ui \
    GridNetConfigPage.ui \
    LayerMappingWidget.ui \
    LayerSourceDialog.ui \
    LayoutProperties.ui \
    LayoutViewConfigPage1.ui \
    LayoutViewConfigPage2a.ui \
    LayoutViewConfigPage2b.ui \
    LayoutViewConfigPage2c.ui \
    LayoutViewConfigPage2d.ui \
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
    OpenLayoutModeDialog.ui \
    PropertiesDialog.ui \
    RenameCellDialog.ui \
    ReplaceCellOptionsDialog.ui \
    SaveLayoutOptionsDialog.ui \
    SaveLayoutAsOptionsDialog.ui \
    SelectStippleForm.ui \
    TipDialog.ui \
    UserPropertiesForm.ui \
    UserPropertiesEditForm.ui \
    SpecificLoadLayoutOptionsDialog.ui \
    SelectLineStyleForm.ui \
    LayoutViewConfigPage6a.ui \
    EditLineStylesForm.ui \
    NetlistBrowserPage.ui \
    NetlistBrowserConfigPage.ui \
    NetlistBrowserConfigPage2.ui \
    NetlistBrowserDialog.ui \
    NetInfoDialog.ui \
    NetExportDialog.ui \
    SelectCellViewForm.ui \
    LayoutStatistics.ui \

  RESOURCES = \
    laybasicResources.qrc \
    layLayoutStatistics.qrc \

  SOURCES = \
    gsiDeclLayDialogs.cc \
    gsiDeclLayMenu.cc \
    gsiDeclLayNetlistBrowserDialog.cc \
    gsiDeclLayStream.cc \
    gtf.cc \
    layAbstractMenu.cc \
    layBackgroundAwareTreeStyle.cc \
    layBitmapsToImage.cc \
    layBookmarkList.cc \
    layBookmarkManagementForm.cc \
    layBookmarksView.cc \
    layBrowseInstancesForm.cc \
    layBrowseShapesForm.cc \
    layBrowser.cc \
    layBrowserDialog.cc \
    layBrowserPanel.cc \
    layBusy.cc \
    layCellSelectionForm.cc \
    layCellTreeModel.cc \
    layConfigurationDialog.cc \
    layConverters.cc \
    layCursor.cc \
    layDialogs.cc \
    layEditLineStyleWidget.cc \
    layEditLineStylesForm.cc \
    layEditStippleWidget.cc \
    layEditStipplesForm.cc \
    layEditorOptionsFrame.cc \
    layEditorOptionsPage.cc \
    layEditorOptionsPages.cc \
    layEditorServiceBase.cc \
    layFileDialog.cc \
    layGenericSyntaxHighlighter.cc \
    layGridNetConfigPage.cc \
    layHierarchyControlPanel.cc \
    layIndexedNetlistModel.cc \
    layItemDelegates.cc \
    layLayerControlPanel.cc \
    layLayerMappingWidget.cc \
    layLayerToolbox.cc \
    layLayerTreeModel.cc \
    layLayoutPropertiesForm.cc \
    layLayoutStatisticsForm.cc \
    layLayoutViewConfigPages.cc \
    layLayoutViewFunctions.cc \
    layLibrariesView.cc \
    layLoadLayoutOptionsDialog.cc \
    layNetExportDialog.cc \
    layNetInfoDialog.cc \
    layNetlistBrowser.cc \
    layNetlistBrowserDialog.cc \
    layNetlistBrowserModel.cc \
    layNetlistBrowserPage.cc \
    layNetlistBrowserTreeModel.cc \
    layNetlistCrossReferenceModel.cc \
    layPixelBuffer.cc \
    layPixelBufferPainter.cc \
    layPluginConfigPage.cc \
    layProperties.cc \
    layPropertiesDialog.cc \
    layQtTools.cc \
    laySaveLayoutOptionsDialog.cc \
    laySelectCellViewForm.cc \
    laySelectLineStyleForm.cc \
    laySelectStippleForm.cc \
    laySelector.cc \
    layStream.cc \
    layTechnology.cc \
    layTipDialog.cc \
    layWidgets.cc \
    layZoomBox.cc \
    rdbInfoWidget.cc \
    rdbMarkerBrowser.cc \
    rdbMarkerBrowserDialog.cc \
    rdbMarkerBrowserPage.cc \

  HEADERS = \
    gtf.h \
    layAbstractMenu.h \
    layBackgroundAwareTreeStyle.h \
    layBitmap.h \
    layBitmapsToImage.h \
    layBookmarkList.h \
    layBookmarkManagementForm.h \
    layBookmarksView.h \
    layBrowseInstancesForm.h \
    layBrowseShapesForm.h \
    layBrowser.h \
    layBrowserDialog.h \
    layBrowserPanel.h \
    layBusy.h \
    layCellSelectionForm.h \
    layCellTreeModel.h \
    layConfigurationDialog.h \
    layConverters.h \
    layColor.h \
    layCursor.h \
    layDialogs.h \
    layEditLineStyleWidget.h \
    layEditLineStylesForm.h \
    layEditStippleWidget.h \
    layEditStipplesForm.h \
    layEditorOptionsFrame.h \
    layEditorOptionsPage.h \
    layEditorOptionsPages.h \
    layEditorServiceBase.h \
    layFileDialog.h \
    layGenericSyntaxHighlighter.h \
    layGridNetConfigPage.h \
    layHierarchyControlPanel.h \
    layIndexedNetlistModel.h \
    layItemDelegates.h \
    layLayerControlPanel.h \
    layLayerMappingWidget.h \
    layLayerToolbox.h \
    layLayerTreeModel.h \
    layLayoutPropertiesForm.h \
    layLayoutStatisticsForm.h \
    layLayoutViewConfigPages.h \
    layLayoutViewFunctions.h \
    layLibrariesView.h \
    layLoadLayoutOptionsDialog.h \
    layNetExportDialog.h \
    layNetInfoDialog.h \
    layNetlistBrowser.h \
    layNetlistBrowserDialog.h \
    layNetlistBrowserModel.h \
    layNetlistBrowserPage.h \
    layNetlistBrowserTreeModel.h \
    layNetlistCrossReferenceModel.h \
    layPixelBuffer.h \
    layPixelBufferPainter.h \
    layPluginConfigPage.h \
    layProperties.h \
    layPropertiesDialog.h \
    layQtTools.h \
    laySaveLayoutOptionsDialog.h \
    laySelectCellViewForm.h \
    laySelectLineStyleForm.h \
    laySelectStippleForm.h \
    laySelector.h \
    layStream.h \
    layTechnology.h \
    layTipDialog.h \
    layWidgets.h \
    layZoomBox.h \
    laybasicConfig.h \
    rdbInfoWidget.h \
    rdbMarkerBrowser.h \
    rdbMarkerBrowserDialog.h \
    rdbMarkerBrowserPage.h \

}

SOURCES += \
    gsiDeclLayLayers.cc \
    gsiDeclLayLayoutView.cc \
    gsiDeclLayMarker.cc \
    gsiDeclLayPlugin.cc \
    gsiDeclLayPixelBuffer.cc \
    layAnnotationShapes.cc \
    layBitmap.cc \
    layBitmapRenderer.cc \
    layCellView.cc \
    layColor.cc \
    layColorPalette.cc \
    layDispatcher.cc \
    layDisplayState.cc \
    layDitherPattern.cc \
    layDrawing.cc \
    layEditable.cc \
    layFinder.cc \
    layGridNet.cc \
    layFixedFont.cc \
    layLayoutCanvas.cc \
    layLayoutView.cc \
    layLineStylePalette.cc \
    layLineStyles.cc \
    layMarker.cc \
    layMouseTracker.cc \
    layMove.cc \
    layNetColorizer.cc \
    layObjectInstPath.cc \
    layParsedLayerSource.cc \
    layPlugin.cc \
    layRedrawLayerInfo.cc \
    layRedrawThread.cc \
    layRedrawThreadCanvas.cc \
    layRedrawThreadWorker.cc \
    layRenderer.cc \
    layRubberBox.cc \
    laySnap.cc \
    layStipplePalette.cc \
    layCanvasPlane.cc \
    layLayoutViewBase.cc \
    layLayerProperties.cc \
    layViewObject.cc \
    layViewOp.cc \
    layViewport.cc \

HEADERS += \
    layAnnotationShapes.h \
    layBitmap.h \
    layBitmapRenderer.h \
    layCellView.h \
    layColorPalette.h \
    layDispatcher.h \
    layDisplayState.h \
    layDitherPattern.h \
    layDrawing.h \
    layEditable.h \
    layLayoutCanvas.h \
    layLayoutView.h \
    layFinder.h \
    layFixedFont.h \
    layGridNet.h \
    layLayoutViewBase.h \
    layLineStylePalette.h \
    layLineStyles.h \
    layMarker.h \
    layMouseTracker.h \
    layMove.h \
    layNetColorizer.h \
    layObjectInstPath.h \
    layParsedLayerSource.h \
    layPlugin.h \
    layRedrawLayerInfo.h \
    layRedrawThread.h \
    layRedrawThreadCanvas.h \
    layRedrawThreadWorker.h \
    layRenderer.h \
    layRubberBox.h \
    laySnap.h \
    layStipplePalette.h \
    layLayerProperties.h \
    layCanvasPlane.h \
    layViewObject.h \
    layViewOp.h \
    layViewport.h \
    laybasicCommon.h \


INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LYM_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LYM_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb -lklayout_lym

INCLUDEPATH += $$QTBASIC_INC
DEPENDPATH += $$QTBASIC_INC

equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lklayout_qtbasic -lklayout_QtGui -lklayout_QtCore
  greaterThan(QT_MAJOR_VERSION, 4) {
    LIBS += -lklayout_QtWidgets
  }
}

