
include($$PWD/../../klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui \
  QtWidgets

QtGui.depends += QtCore
QtWidgets.depends += QtGui

!equals(HAVE_QT_NETWORK, "0") {
  SUBDIRS += QtNetwork
  QtNetwork.depends += QtCore
}

!equals(HAVE_QT_SQL, "0") {
  SUBDIRS += QtSql
  QtSql.depends += QtCore
}

!equals(HAVE_QT_SVG, "0") {
  SUBDIRS += QtSvg
  QtSvg.depends += QtCore QtWidgets
}

!equals(HAVE_QT_PRINTSUPPORT, "0") {
  SUBDIRS += QtPrintSupport
  QtPrintSupport.depends += QtCore QtWidgets
}

!equals(HAVE_QT_MULTIMEDIA, "0") {
  SUBDIRS += QtMultimedia
  QtMultimedia.depends += QtCore QtWidgets QtNetwork
}

!equals(HAVE_QT_DESIGNER, "0") {
  SUBDIRS += QtDesigner
  QtDesigner.depends += QtCore
}

!equals(HAVE_QT_XML, "0") {
  SUBDIRS += QtXml QtXmlPatterns
  QtXmlPatterns.depends += QtCore
  QtXml.depends += QtCore
}

!equals(HAVE_QT_UITOOLS, "0") {
  SUBDIRS += QtUiTools
  QtUiTools.depends += QtCore
}
