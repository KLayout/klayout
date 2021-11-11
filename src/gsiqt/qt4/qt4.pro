
include($$PWD/../../klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui

QtGui.depends += QtCore

!equals(HAVE_QT_NETWORK, "0") {
  SUBDIRS += QtNetwork
  QtNetwork.depends += QtCore
}

!equals(HAVE_QT_SQL, "0") {
  SUBDIRS += QtSql
  QtSql.depends += QtCore
}

!equals(HAVE_QT_DESIGNER, "0") {
  SUBDIRS += QtDesigner
  QtDesigner.depends += QtCore
}

!equals(HAVE_QT_XML, "0") {
  SUBDIRS += QtXml
  QtXml.depends += QtCore
}

!equals(HAVE_QT_UITOOLS, "0") {
  SUBDIRS += QtUiTools
  QtUiTools.depends += QtCore
}

