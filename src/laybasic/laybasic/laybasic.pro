
DESTDIR = $$OUT_PWD/../..
TARGET = klayout_laybasic

include($$PWD/../../lib.pri)

DEFINES += MAKE_LAYBASIC_LIBRARY

!equals(HAVE_QT, 0) {

  FORMS = \

  SOURCES = \
    gtf.cc \
    layPluginConfigPage.cc \
    layProperties.cc \
    layDragDropData.cc \
    layCursor.cc \

  HEADERS = \
    gtf.h \
    layPluginConfigPage.h \
    layProperties.h \
    layDragDropData.h \
    layCursor.h \

}

SOURCES += \
  gsiDeclLayLayers.cc \
  gsiDeclLayLayoutViewBase.cc \
  gsiDeclLayMarker.cc \
  gsiDeclLayMenu.cc \
  gsiDeclLayPlugin.cc \
  gsiDeclLayTlAdded.cc \
  gsiDeclLayRdbAdded.cc \
  layAbstractMenu.cc \
  layLayoutViewConfig.cc \
  laybasicForceLink.cc \
  layAnnotationShapes.cc \
  layBitmap.cc \
  layBitmapRenderer.cc \
  layBitmapsToImage.cc \
  layBookmarkList.cc \
  layCellView.cc \
  layColorPalette.cc \
  layConverters.cc \
  layDispatcher.cc \
  layDisplayState.cc \
  layDitherPattern.cc \
  layDrawing.cc \
  layEditable.cc \
  layEditorServiceBase.cc \
  layFinder.cc \
  layTextInfo.cc \
  layFixedFont.cc \
  layLayoutCanvas.cc \
  layLineStylePalette.cc \
  layLineStyles.cc \
  layMarker.cc \
  layMouseTracker.cc \
  layMove.cc \
  layNativePlugin.cc \
  layNetColorizer.cc \
  layObjectInstPath.cc \
  layParsedLayerSource.cc \
  layPixelBufferPainter.cc \
  layPlugin.cc \
  layRedrawLayerInfo.cc \
  layRedrawThread.cc \
  layRedrawThreadCanvas.cc \
  layRedrawThreadWorker.cc \
  layRenderer.cc \
  layRubberBox.cc \
  laySelector.cc \
  laySnap.cc \
  layStream.cc \
  layStipplePalette.cc \
  layCanvasPlane.cc \
  layLayoutViewBase.cc \
  layLayerProperties.cc \
  layViewObject.cc \
  layViewOp.cc \
  layViewport.cc \
  layZoomBox.cc \
  layUtils.cc \

HEADERS += \
  laybasicConfig.h \
  laybasicForceLink.h \
  layAbstractMenu.h \
  layAnnotationShapes.h \
  layBitmap.h \
  layBitmapRenderer.h \
  layBitmapsToImage.h \
  layBookmarkList.h \
  layCellView.h \
  layColorPalette.h \
  layConverters.h \
  layDispatcher.h \
  layDisplayState.h \
  layDitherPattern.h \
  layDrawing.h \
  layEditable.h \
  layEditorServiceBase.h \
  layLayoutCanvas.h \
  layFinder.h \
  layTextInfo.h \
  layFixedFont.h \
  layLayoutViewBase.h \
  layLineStylePalette.h \
  layLineStyles.h \
  layMarker.h \
  layMouseTracker.h \
  layMove.h \
  layNativePlugin.h \
  layNetColorizer.h \
  layObjectInstPath.h \
  layParsedLayerSource.h \
  layPixelBufferPainter.h \
  layPlugin.h \
  layRedrawLayerInfo.h \
  layRedrawThread.h \
  layRedrawThreadCanvas.h \
  layRedrawThreadWorker.h \
  layRenderer.h \
  layRubberBox.h \
  laySelector.h \
  laySnap.h \
  layStream.h \
  layStipplePalette.h \
  layLayerProperties.h \
  layCanvasPlane.h \
  layViewObject.h \
  layViewOp.h \
  layViewport.h \
  layZoomBox.h \
  layUtils.h \
  laybasicCommon.h \


INCLUDEPATH += $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC
DEPENDPATH += $$TL_INC $$GSI_INC $$DB_INC $$RDB_INC
LIBS += -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb 

INCLUDEPATH += $$QTBASIC_INC
DEPENDPATH += $$QTBASIC_INC

equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lklayout_qtbasic -lklayout_QtGui -lklayout_QtCore
  greaterThan(QT_MAJOR_VERSION, 4) {
    LIBS += -lklayout_QtWidgets
  }
}

