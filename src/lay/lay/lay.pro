
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_lay

include($$PWD/../../lib.pri)

DEFINES += MAKE_LAY_LIBRARY

HEADERS = \
    layApplication.h \
    layClipDialog.h \
    layControlWidgetStack.h \
    layCrashMessage.h \
    layEnhancedTabBar.h \
    layFillDialog.h \
    layGSIHelpProvider.h \
    layHelpAboutDialog.h \
    layHelpDialog.h \
    layHelpProvider.h \
    layHelpSource.h \
    layLogViewerDialog.h \
    layMacroEditorDialog.h \
    layMacroEditorPage.h \
    layMacroEditorTree.h \
    layMacroPropertiesDialog.h \
    layMacroVariableView.h \
    layMainConfigPages.h \
    layMainWindow.h \
    layNavigator.h \
    layProgress.h \
    layProgressDialog.h \
    layProgressWidget.h \
    layResourceHelpProvider.h \
    layRuntimeErrorForm.h \
    layReaderErrorForm.h \
    laySaltParsedURL.h \
    laySearchReplaceConfigPage.h \
    laySearchReplaceDialog.h \
    laySearchReplacePropertiesWidgets.h \
    laySession.h \
    laySettingsForm.h \
    layTechSetupDialog.h \
    layTextProgress.h \
    layTextProgressDelegate.h \
    layVersion.h \
    layCommon.h \
    layConfig.h \
    layMacroController.h \
    layTechnologyController.h \
    laySalt.h \
    laySaltGrain.h \
    laySaltGrains.h \
    laySaltManagerDialog.h \
    laySaltGrainDetailsTextWidget.h \
    laySaltGrainPropertiesDialog.h \
    laySaltDownloadManager.h \
    laySaltModel.h \
    laySaltController.h \
    laySignalHandler.h \
    layLibraryController.h \
    layFontController.h \
    laySystemPaths.h \
    layMacroEditorSetupPage.h \
    layPasswordDialog.h \
    layForceLink.h \
    layInit.h \
    layViewWidgetStack.h

FORMS = \
    ClipDialog.ui \
    CrashMessage.ui \
    Console.ui \
    DeleteModeDialog.ui \
    FillDialog.ui \
    HelpAboutDialog.ui \
    HelpDialog.ui \
    LogViewerDialog.ui \
    MacroEditorDialog.ui \
    MacroPropertiesDialog.ui \
    MacroTemplateSelectionDialog.ui \
    MainConfigPage.ui \
    MainConfigPage2.ui \
    MainConfigPage3.ui \
    MainConfigPage4.ui \
    MainConfigPage5.ui \
    MainConfigPage6.ui \
    ReplacePropertiesBox.ui \
    ReplacePropertiesInstance.ui \
    ReplacePropertiesPath.ui \
    ReplacePropertiesShape.ui \
    ReplacePropertiesText.ui \
    RuntimeErrorForm.ui \
    ReaderErrorForm.ui \
    SearchPropertiesBox.ui \
    SearchPropertiesInstance.ui \
    SearchPropertiesPath.ui \
    SearchPropertiesShape.ui \
    SearchPropertiesText.ui \
    SearchReplaceConfigPage.ui \
    SearchReplaceDialog.ui \
    SettingsForm.ui \
    TechBaseEditorPage.ui \
    TechComponentSetupDialog.ui \
    TechLayerMappingEditorPage.ui \
    TechMacrosPage.ui \
    TechSetupDialog.ui \
    TechLoadOptionsEditorPage.ui \
    TechSaveOptionsEditorPage.ui \
    MainConfigPage7.ui \
    SaltManagerDialog.ui \
    SaltGrainPropertiesDialog.ui \
    SaltGrainTemplateSelectionDialog.ui \
    SaltManagerInstallConfirmationDialog.ui \
    CustomizeMenuConfigPage.ui \
    MacroEditorSetupPage.ui \
    PasswordDialog.ui

SOURCES = \
    gsiDeclLayApplication.cc \
    gsiDeclLayHelpDialog.cc \
    gsiDeclLayMainWindow.cc \
    layApplication.cc \
    layClipDialog.cc \
    layControlWidgetStack.cc \
    layCrashMessage.cc \
    layEnhancedTabBar.cc \
    layFillDialog.cc \
    layGSIHelpProvider.cc \
    layHelpAboutDialog.cc \
    layHelpDialog.cc \
    layHelpProvider.cc \
    layHelpSource.cc \
    layLogViewerDialog.cc \
    layMacroEditorDialog.cc \
    layMacroEditorPage.cc \
    layMacroEditorTree.cc \
    layMacroPropertiesDialog.cc \
    layMacroVariableView.cc \
    layMainConfigPages.cc \
    layMainWindow.cc \
    layNavigator.cc \
    layProgress.cc \
    layProgressDialog.cc \
    layProgressWidget.cc \
    layResourceHelpProvider.cc \
    layRuntimeErrorForm.cc \
    layReaderErrorForm.cc \
    laySaltParsedURL.cc \
    laySearchReplaceConfigPage.cc \
    laySearchReplaceDialog.cc \
    laySearchReplacePlugin.cc \
    laySearchReplacePropertiesWidgets.cc \
    laySession.cc \
    laySettingsForm.cc \
    layTechSetupDialog.cc \
    layTextProgress.cc \
    layTextProgressDelegate.cc \
    layVersion.cc \
    layMacroController.cc \
    layTechnologyController.cc \
    laySalt.cc \
    laySaltGrain.cc \
    laySaltGrains.cc \
    laySaltManagerDialog.cc \
    laySaltGrainDetailsTextWidget.cc \
    laySaltGrainPropertiesDialog.cc \
    laySaltDownloadManager.cc \
    laySaltModel.cc \
    laySaltController.cc \
    laySignalHandler.cc \
    layLibraryController.cc \
    layFontController.cc \
    laySystemPaths.cc \
    layMacroEditorSetupPage.cc \
    layPasswordDialog.cc \
    layForceLink.cc \
    layInit.cc \
    layViewWidgetStack.cc

RESOURCES = layBuildInMacros.qrc \
    layMacroTemplates.qrc \
    laySyntaxHighlighters.qrc \
    laySaltTemplates.qrc \

INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LAYBASIC_INC $$LAYUI_INC $$LAYVIEW_INC $$ANT_INC $$IMG_INC $$EDT_INC $$LYM_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LAYBASIC_INC $$LAYUI_INC $$LAYVIEW_INC $$ANT_INC $$IMG_INC $$EDT_INC $$LYM_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb -lklayout_lym -lklayout_laybasic -lklayout_layview -lklayout_layui -lklayout_ant -lklayout_img -lklayout_edt

INCLUDEPATH += $$QTBASIC_INC
DEPENDPATH += $$QTBASIC_INC

equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lklayout_qtbasic -lklayout_QtGui -lklayout_QtCore
  !equals(HAVE_QT_XML, "0") {
    LIBS += -lklayout_QtXml
  }
  greaterThan(QT_MAJOR_VERSION, 4) {
    LIBS += -lklayout_QtWidgets
  }
}

INCLUDEPATH += $$RBA_INC
DEPENDPATH += $$RBA_INC

equals(HAVE_RUBY, "1") {
  LIBS += -lklayout_rba
} else {
  LIBS += -lklayout_rbastub
}

INCLUDEPATH += $$PYA_INC
DEPENDPATH += $$PYA_INC

equals(HAVE_PYTHON, "1") {
  LIBS += -lklayout_pya
} else {
  LIBS += -lklayout_pyastub
}
