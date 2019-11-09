
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_lay

include($$PWD/../../lib.pri)

DEFINES += MAKE_LAY_LIBRARY

HEADERS = \
    layApplication.h \
    layClipDialog.h \
    layCrashMessage.h \
    layFillDialog.h \
    layGSIHelpProvider.h \
    layHelpDialog.h \
    layHelpProvider.h \
    layHelpSource.h \
    layLayoutStatisticsForm.h \
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
    layProgressWidget.h \
    layResourceHelpProvider.h \
    layRuntimeErrorForm.h \
    laySearchReplaceConfigPage.h \
    laySearchReplaceDialog.h \
    laySearchReplacePropertiesWidgets.h \
    laySelectCellViewForm.h \
    laySession.h \
    laySettingsForm.h \
    layTechSetupDialog.h \
    layTextProgress.h \
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
    layNativePlugin.h \
    laySystemPaths.h \
    layMacroEditorSetupPage.h \
    layPasswordDialog.h \
    layForceLink.h \
    layInit.h

FORMS = \
    ClipDialog.ui \
    CrashMessage.ui \
    Console.ui \
    DeleteModeDialog.ui \
    FillDialog.ui \
    HelpAboutDialog.ui \
    LayoutStatistics.ui \
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
    SearchPropertiesBox.ui \
    SearchPropertiesInstance.ui \
    SearchPropertiesPath.ui \
    SearchPropertiesShape.ui \
    SearchPropertiesText.ui \
    SearchReplaceConfigPage.ui \
    SearchReplaceDialog.ui \
    SelectCellViewForm.ui \
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
    layCrashMessage.cc \
    layFillDialog.cc \
    layGSIHelpProvider.cc \
    layHelpDialog.cc \
    layHelpProvider.cc \
    layHelpSource.cc \
    layLayoutStatisticsForm.cc \
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
    layProgressWidget.cc \
    layResourceHelpProvider.cc \
    layRuntimeErrorForm.cc \
    laySearchReplaceConfigPage.cc \
    laySearchReplaceDialog.cc \
    laySearchReplacePlugin.cc \
    laySearchReplacePropertiesWidgets.cc \
    laySelectCellViewForm.cc \
    laySession.cc \
    laySettingsForm.cc \
    layTechSetupDialog.cc \
    layTextProgress.cc \
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
    layNativePlugin.cc \
    laySystemPaths.cc \
    layMacroEditorSetupPage.cc \
    layPasswordDialog.cc \
    layForceLink.cc \
    layInit.cc

RESOURCES = layBuildInMacros.qrc \
    layHelpResources.qrc \
    layLayoutStatistics.qrc \
    layMacroTemplates.qrc \
    layResources.qrc \
    laySaltTemplates.qrc

INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LAYBASIC_INC $$ANT_INC $$IMG_INC $$EDT_INC $$LYM_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC $$LAYBASIC_INC $$ANT_INC $$IMG_INC $$EDT_INC $$LYM_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb -lklayout_lym -lklayout_laybasic -lklayout_ant -lklayout_img -lklayout_edt

win32 {
  # for stack trace support:
  # lpsapi for GetModuleFileName and others
  # dbghelp for SymFromAddr and other
  LIBS += -lpsapi -ldbghelp
}

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic/laybasic
DEPENDPATH += $$DESTDIR/laybasic/laybasic

INCLUDEPATH += $$QTBASIC_INC
DEPENDPATH += $$QTBASIC_INC

equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lklayout_qtbasic -lklayout_QtGui -lklayout_QtXml -lklayout_QtCore
  equals(HAVE_QT5, "1") {
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
