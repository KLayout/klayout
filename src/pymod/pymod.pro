
TEMPLATE = subdirs
SUBDIRS = \
  db \
  tl \
  lay \

equals(HAVE_QTBINDINGS, "1") {
  equals(HAVE_QT5, "1") {

    SUBDIRS += \
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

  } else {

    SUBDIRS += \
      QtCore \
      QtGui \
      QtXml \
      QtSql \
      QtNetwork \
      QtDesigner

  }
}
