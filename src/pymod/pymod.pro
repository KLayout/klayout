
include($$PWD/../klayout.pri)

TEMPLATE = subdirs
SUBDIRS = \
  db \
  tl \
  rdb \
  lib \

!equals(HAVE_QT, "0") {

  SUBDIRS += lay

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

    equals(HAVE_QT_UITOOLS, "1") {
      SUBDIRS += QtUiTools
    }

  }
}

ALL_DIRS = $$SUBDIRS

SUBDIRS += unit_tests
SUBDIRS += bridge_sample

unit_tests.depends += $$ALL_DIRS bridge_sample
