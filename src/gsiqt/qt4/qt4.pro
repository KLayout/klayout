
TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui \
  QtXml \
  QtSql \
  QtNetwork \
  QtDesigner

QtGui.depends += QtCore
QtXml.depends += QtCore
QtSql.depends += QtCore
QtNetwork.depends += QtCore
QtDesigner.depends += QtGui QtCore

