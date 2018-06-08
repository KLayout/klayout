
TEMPLATE = subdirs

SUBDIRS = \
  QtCore \
  QtGui \
  QtNetwork \
  QtSql \
  QtWidgets \
  QtDesigner \
  QtMultimedia \
  QtPrintSupport \
  QtSvg \
  QtXmlPatterns \
  QtXml

QtGui.depends += QtCore
QtNetwork.depends += QtCore
QtSql.depends += QtCore
QtWidgets.depends += QtGui
QtDesigner.depends += QtCore
QtMultimedia.depends += QtCore QtWidgets QtNetwork
QtPrintSupport.depends += QtCore QtWidgets
QtSvg.depends += QtCore QtWidgets
QtXmlPatterns.depends += QtCore
QtXml.depends += QtCore
