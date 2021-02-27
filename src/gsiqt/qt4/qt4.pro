
include($$PWD/../../klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui \
  QtXml \
  QtSql \
  QtNetwork \
  QtDesigner 

QtGui.depends += QtCore
QtNetwork.depends += QtCore
QtSql.depends += QtCore
QtDesigner.depends += QtCore
QtXml.depends += QtCore

equals(HAVE_QT_UITOOLS, "1") {
  SUBDIRS += QtUiTools
  QtUiTools.depends += QtCore
}

