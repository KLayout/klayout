
TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui \
  QtXml \
  QtSql \
  QtNetwork \
  QtDesigner \
  QtUiTools

QtGui.depends += QtCore
QtNetwork.depends += QtCore
QtSql.depends += QtCore
QtDesigner.depends += QtCore
QtXml.depends += QtCore
QtUiTools.depends += QtCore
