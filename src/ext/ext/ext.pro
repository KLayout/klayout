
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_ext

include($$PWD/../../lib.pri)

DEFINES += MAKE_EXT_LIBRARY

HEADERS += \
  extBooleanOperationsDialogs.h \
  extDEFImporter.h \
  extDiffToolDialog.h \
  extGerberDrillFileReader.h \
  extGerberImportDialog.h \
  extGerberImporter.h \
  extLEFDEFImportDialogs.h \
  extLEFDEFImporter.h \
  extLEFImporter.h \
  extNetTracer.h \
  extNetTracerConfig.h \
  extNetTracerDialog.h \
  extNetTracerIO.h \
  extRS274XApertures.h \
  extRS274XReader.h \
  extStreamImportDialog.h \
  extStreamImporter.h \
  extXORToolDialog.h \
    extCommon.h \
    extForceLink.h \
    extXORProgress.h

FORMS += \ 
  BooleanOptionsDialog.ui \
  DiffToolDialog.ui \
  GerberImportDialog.ui \
  LEFDEFImportOptionsDialog.ui \
  LEFDEFTechnologyComponentEditor.ui \
  NetTracerConfigPage.ui \
  NetTracerDialog.ui \
  NetTracerTechComponentEditor.ui \
  SizingOptionsDialog.ui \
  StreamImportDialog.ui \
  MergeOptionsDialog.ui \

SOURCES += \
  extBooleanOperationsDialogs.cc \
  extBooleanOperationsPlugin.cc \
  extDEFImporter.cc \
  extDiffPlugin.cc \
  extDiffToolDialog.cc \
  extForceLink.cc \
  extGerberDrillFileReader.cc \
  extGerberImport.cc \
  extGerberImportDialog.cc \
  extGerberImporter.cc \
  extLEFDEFImport.cc \
  extLEFDEFImportDialogs.cc \
  extLEFDEFImporter.cc \
  extLEFImporter.cc \
  extNetTracer.cc \
  extNetTracerConfig.cc \
  extNetTracerDialog.cc \
  extNetTracerIO.cc \
  extNetTracerPlugin.cc \
  extRS274XApertures.cc \
  extRS274XReader.cc \
  extStreamImport.cc \
  extStreamImportDialog.cc \
  extStreamImporter.cc \
  extXORPlugin.cc \
  extXORToolDialog.cc \
    extLEFDEFPlugin.cc \
    extXORProgress.cc

INCLUDEPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$LAY_INC $$DB_INC $$RDB_INC $$ANT_INC $$EDT_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$LAYBASIC_INC $$LAY_INC $$DB_INC $$RDB_INC $$ANT_INC $$EDT_INC

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/ext/ext $$DESTDIR/laybasic/laybasic
DEPENDPATH += $$DESTDIR/ext/ext $$DESTDIR/laybasic/laybasic

LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_db -lklayout_rdb -lklayout_ant -lklayout_edt

# TODO: ideally this should not be there:
INCLUDEPATH += $$DESTDIR/lay/lay
DEPENDPATH += $$DESTDIR/lay/lay
LIBS += -L$$DESTDIR -lklayout_lay
