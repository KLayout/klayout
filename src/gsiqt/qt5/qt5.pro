
include($$PWD/../../klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui \
  QtWidgets

QtGui.depends += QtCore
QtWidgets.depends += QtGui

equals(HAVE_QT_NETWORK, "1") {
  SUBDIRS += QtNetwork
  QtNetwork.depends += QtCore
}

equals(HAVE_QT_SQL, "1") {
  SUBDIRS += QtSql
  QtSql.depends += QtCore
}

equals(HAVE_QT_SVG, "1") {
  SUBDIRS += QtSvg
  QtSvg.depends += QtCore QtWidgets
}

equals(HAVE_QT_PRINTSUPPORT, "1") {
  SUBDIRS += QtPrintSupport
  QtPrintSupport.depends += QtCore QtWidgets
}

equals(HAVE_QT_MULTIMEDIA, "1") {
  SUBDIRS += QtMultimedia
  QtMultimedia.depends += QtCore QtWidgets QtNetwork
}

equals(HAVE_QT_DESIGNER, "1") {
  SUBDIRS += QtDesigner
  QtDesigner.depends += QtCore
}

equals(HAVE_QT_XML, "1") {
  SUBDIRS += QtXml QtXmlPatterns
  QtXmlPatterns.depends += QtCore
  QtXml.depends += QtCore
}

equals(HAVE_QT_UITOOLS, "1") {
  SUBDIRS += QtUiTools
  QtUiTools.depends += QtCore
}
