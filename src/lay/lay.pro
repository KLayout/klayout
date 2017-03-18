
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

DEFINES += MAKE_LAY_LIBRARY

TEMPLATE = lib

HEADERS = \
  layApplication.h \
  layClipDialog.h \
  layCrashMessage.h \
  layFillDialog.h \
  layGenericSyntaxHighlighter.h \
  layGSIHelpProvider.h \
  layHelpDialog.h \
  layHelpProvider.h \
  layHelpSource.h \
  layLayoutStatisticsForm.h \
  layLogViewerDialog.h \
  layMacro.h \
  layMacroEditorDialog.h \
  layMacroEditorPage.h \
  layMacroEditorSetupDialog.h \
  layMacroEditorTree.h \
  layMacroInterpreter.h \
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
  layTechnologySelector.h \
  layTechSetupDialog.h \
  layTextProgress.h \
  layVersion.h \
    layCommon.h \
    layConfig.h \
    laySalt.h \
    laySaltGrain.h \
    laySaltGrains.h \
    laySaltManagerDialog.h \
    laySaltGrainDetailsTextWidget.h

FORMS = \
  ClipDialog.ui \
  CrashMessage.ui \
  Console.ui \
  DeleteModeDialog.ui \
  FillDialog.ui \
  HelpAboutDialog.ui \
  KeyBindingsConfigPage.ui \
  LayoutStatistics.ui \
  LogViewerDialog.ui \
  MacroEditorDialog.ui \
  MacroEditorSetupDialog.ui \
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
  XORToolDialog.ui \
    TechLoadOptionsEditorPage.ui \
    TechSaveOptionsEditorPage.ui \
    MainConfigPage7.ui \
    SaltManagerDialog.ui

SOURCES = \
  gsiDeclLayApplication.cc \
  gsiDeclLayHelpDialog.cc \
  gsiDeclLayMacro.cc \
  gsiDeclLayMainWindow.cc \
  layApplication.cc \
  layClipDialog.cc \
  layCrashMessage.cc \
  layFillDialog.cc \
  layGenericSyntaxHighlighter.cc \
  layGSIHelpProvider.cc \
  layHelpDialog.cc \
  layHelpProvider.cc \
  layHelpSource.cc \
  layLayoutStatisticsForm.cc \
  layLogViewerDialog.cc \
  layMacro.cc \
  layMacroEditorDialog.cc \
  layMacroEditorPage.cc \
  layMacroEditorSetupDialog.cc \
  layMacroEditorTree.cc \
  layMacroInterpreter.cc \
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
  layTechnologySelector.cc \
  layTechSetupDialog.cc \
  layTextProgress.cc \
  layVersion.cc \
    laySalt.cc \
    laySaltGrain.cc \
    laySaltGrains.cc \
    laySaltManagerDialog.cc \
    laySaltGrainDetailsTextWidget.cc

RESOURCES = layBuildInMacros.qrc \
  layHelpResources.qrc \
  layLayoutStatistics.qrc \
  layMacroTemplates.qrc \
  layResources.qrc \

INCLUDEPATH += ../tl ../gsi ../db ../rdb ../laybasic ../ant ../img ../edt
DEPENDPATH += ../tl ../gsi ../db ../rdb ../laybasic ../ant ../img ../edt
LIBS += -L$$DESTDIR -ltl -lgsi -ldb -lrdb -llaybasic -lant -limg -ledt

win32 {
  # for stack trace support:
  # lpsapi for GetModuleFileName and others
  # dbghelp for SymFromAddr and other
  LIBS += -lpsapi -ldbghelp
}

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic
DEPENDPATH += $$DESTDIR/laybasic

INCLUDEPATH += ../gsiqt
DEPENDPATH += ../gsiqt

equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lgsiqt
}

equals(HAVE_RUBY, "1") {
  INCLUDEPATH += ../rba
  DEPENDPATH += ../rba
  LIBS += -lrba 
} else {
  INCLUDEPATH += ../rbastub
  DEPENDPATH += ../rbastub
  LIBS += -lrbastub 
}

equals(HAVE_PYTHON, "1") {
  INCLUDEPATH += ../pya
  DEPENDPATH += ../pya
  LIBS += -lpya 
} else {
  INCLUDEPATH += ../pyastub
  DEPENDPATH += ../pyastub
  LIBS += -lpyastub
}

