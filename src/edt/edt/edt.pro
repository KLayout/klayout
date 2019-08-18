
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_edt

include($$PWD/../../lib.pri)

DEFINES += MAKE_EDT_LIBRARY

HEADERS = \
  edtConfig.h \
  edtDialogs.h \
  edtEditorOptionsPages.h \
  edtInstPropertiesPage.h \
  edtMainService.h \
  edtPartialService.h \
  edtPCellParametersPage.h \
  edtPlugin.h \
  edtPropertiesPages.h \
  edtPropertiesPageUtils.h \
  edtService.h \
  edtServiceImpl.h \
  edtUtils.h \
    edtCommon.h \
    edtPCellParametersDialog.h

FORMS = \
  AlignOptionsDialog.ui \
  BoxPropertiesPage.ui \
  CopyModeDialog.ui \
  ChangeLayerOptionsDialog.ui \
  EditablePathPropertiesPage.ui \
  EditorOptionsDialog.ui \
  EditorOptionsGeneric.ui \
  EditorOptionsInst.ui \
  EditorOptionsPath.ui \
  EditorOptionsText.ui \
  InstantiationForm.ui \
  InstPropertiesPage.ui \
  MakeArrayOptionsDialog.ui \
  MakeCellOptionsDialog.ui \
  PathPropertiesPage.ui \
  PolygonPropertiesPage.ui \
  RoundCornerOptionsDialog.ui \
  TextPropertiesPage.ui \
    PCellParametersDialog.ui

SOURCES = \
  edtConfig.cc \
  edtDialogs.cc \
  edtEditorOptionsPages.cc \
  edtInstPropertiesPage.cc \
  edtMainService.cc \
  edtPartialService.cc \
  edtPCellParametersPage.cc \
  edtPlugin.cc \
  edtPropertiesPages.cc \
  edtPropertiesPageUtils.cc \
  edtService.cc \
  edtServiceImpl.cc \
  edtUtils.cc \
  gsiDeclEdt.cc \
    edtPCellParametersDialog.cc

INCLUDEPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic/laybasic
DEPENDPATH += $$DESTDIR/laybasic/laybasic

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_db

