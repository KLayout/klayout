
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

DEFINES += MAKE_EXT_LIBRARY

TEMPLATE = lib

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
    extForceLink.h

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
    extLEFDEFPlugin.cc

INCLUDEPATH += ../tl ../gsi ../laybasic ../lay ../db ../rdb ../ant ../edt
DEPENDPATH += ../tl ../gsi ../laybasic ../lay ../db ../rdb ../ant ../edt

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic $$OUT_PWD/../lay
DEPENDPATH += $$DESTDIR/laybasic $$OUT_PWD/../lay

LIBS += -L$$DESTDIR -ltl -lgsi -llaybasic -ldb -lrdb -llay -lant -ledt

