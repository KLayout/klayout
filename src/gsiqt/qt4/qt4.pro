
include($$PWD/../../klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui

QtGui.depends += QtCore

equals(HAVE_QT_NETWORK, "1") {
  SUBDIRS += QtNetwork
  QtNetwork.depends += QtCore
}

equals(HAVE_QT_SQL, "1") {
  SUBDIRS += QtSql
  QtSql.depends += QtCore
}

equals(HAVE_QT_DESIGNER, "1") {
  SUBDIRS += QtDesigner
  QtDesigner.depends += QtCore
}

equals(HAVE_QT_XML, "1") {
  SUBDIRS += QtXml
  QtXml.depends += QtCore
}

equals(HAVE_QT_UITOOLS, "1") {
  SUBDIRS += QtUiTools
  QtUiTools.depends += QtCore
}

