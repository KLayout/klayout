
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

contains(QT_MODULES, uitools) {
  # Not all distributions have uitools
  SUBDIRS += QtUiTools
  QtUiTools.depends += QtCore
}
