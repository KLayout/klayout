
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_edt

include($$PWD/../../lib.pri)

DEFINES += MAKE_EDT_LIBRARY

!equals(HAVE_QT, "0") {

  HEADERS = \
    edtDialogs.h \
    edtEditorOptionsPages.h \
    edtInstPropertiesPage.h \
    edtPCellParametersPage.h \
    edtPropertiesPages.h \
    edtPropertiesPageUtils.h \
    edtRecentConfigurationPage.h

  FORMS = \
    AlignOptionsDialog.ui \
    BoxPropertiesPage.ui \
    CopyModeDialog.ui \
    ChangeLayerOptionsDialog.ui \
    EditablePathPropertiesPage.ui \
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
    DistributeOptionsDialog.ui \
    EditorOptionsInstPCellParam.ui

  SOURCES = \
    edtDialogs.cc \
    edtEditorOptionsPages.cc \
    edtInstPropertiesPage.cc \
    edtPCellParametersPage.cc \
    edtPropertiesPages.cc \
    edtPropertiesPageUtils.cc \
    edtRecentConfigurationPage.cc

}

HEADERS = \
  edtConfig.h \
  edtMainService.h \
  edtPartialService.h \
  edtPlugin.h \
  edtService.h \
  edtServiceImpl.h \
  edtUtils.h \
  edtCommon.h \
  edtDistribute.h \

SOURCES = \
  edtConfig.cc \
  edtMainService.cc \
  edtPartialService.cc \
  edtPlugin.cc \
  edtService.cc \
  edtServiceImpl.cc \
  edtUtils.cc \
  gsiDeclEdt.cc \
  edtDistribute.cc \

INCLUDEPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$DB_INC

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic/laybasic
DEPENDPATH += $$DESTDIR/laybasic/laybasic

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_db

