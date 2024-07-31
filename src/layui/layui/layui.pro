
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_layui

include($$PWD/../../lib.pri)

DEFINES += MAKE_LAYUI_LIBRARY

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
  LayoutViewConfigPage8.ui \
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
  UndoRedoListForm.ui \
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
  gsiDeclLayNetlistBrowserDialog.cc \
  gsiDeclLayStream.cc \
  layuiForceLink.cc \
  layBackgroundAwareTreeStyle.cc \
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
  layDialogs.cc \
  layEditLineStyleWidget.cc \
  layEditLineStylesForm.cc \
  layEditStippleWidget.cc \
  layEditStipplesForm.cc \
  layEditorOptionsFrame.cc \
  layEditorOptionsPage.cc \
  layEditorOptionsPages.cc \
  layFileDialog.cc \
  layGenericSyntaxHighlighter.cc \
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
  layNetlistLogModel.cc \
  layPropertiesDialog.cc \
  layQtTools.cc \
  laySaveLayoutOptionsDialog.cc \
  laySelectCellViewForm.cc \
  laySelectLineStyleForm.cc \
  laySelectStippleForm.cc \
  layTechnology.cc \
  layTipDialog.cc \
  layWidgets.cc \
  rdbInfoWidget.cc \
  rdbMarkerBrowser.cc \
  rdbMarkerBrowserDialog.cc \
  rdbMarkerBrowserPage.cc \

HEADERS = \
  layuiForceLink.h \
  layBackgroundAwareTreeStyle.h \
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
  layDialogs.h \
  layEditLineStyleWidget.h \
  layEditLineStylesForm.h \
  layEditStippleWidget.h \
  layEditStipplesForm.h \
  layEditorOptionsFrame.h \
  layEditorOptionsPage.h \
  layEditorOptionsPages.h \
  layFileDialog.h \
  layGenericSyntaxHighlighter.h \
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
  layNetlistLogModel.h \
  layPropertiesDialog.h \
  layQtTools.h \
  laySaveLayoutOptionsDialog.h \
  laySelectCellViewForm.h \
  laySelectLineStyleForm.h \
  laySelectStippleForm.h \
  layTechnology.h \
  layTipDialog.h \
  layWidgets.h \
  rdbInfoWidget.h \
  rdbMarkerBrowser.h \
  rdbMarkerBrowserDialog.h \
  rdbMarkerBrowserPage.h \

INCLUDEPATH += $$LAYBASIC_INC $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC
DEPENDPATH += $$LAYBASIC_INC $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC
LIBS += -L$$DESTDIR -lklayout_laybasic -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb

INCLUDEPATH += $$QTBASIC_INC
DEPENDPATH += $$QTBASIC_INC

equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lklayout_qtbasic -lklayout_QtGui -lklayout_QtCore
  greaterThan(QT_MAJOR_VERSION, 4) {
    LIBS += -lklayout_QtWidgets
  }
}

