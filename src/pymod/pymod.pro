
include($$PWD/../klayout.pri)

TEMPLATE = subdirs
SUBDIRS = \
  db \
  pex \
  tl \
  rdb \
  lib \
  lay \
  pya \

!equals(HAVE_QT, "0") {

  equals(HAVE_QTBINDINGS, "1") {

    SUBDIRS += \
      QtCore \
      QtGui

    greaterThan(QT_MAJOR_VERSION, 4) {

      SUBDIRS += QtWidgets

      !equals(HAVE_QT_MULTIMEDIA, "0") {
        SUBDIRS += QtMultimedia
      }

      !equals(HAVE_QT_PRINTSUPPORT, "0") {
        SUBDIRS += QtPrintSupport
      }

      !equals(HAVE_QT_SVG, "0") {
        SUBDIRS += QtSvg
      }

      !equals(HAVE_QT_XML, "0") {
        lessThan(QT_MAJOR_VERSION, 6) {
          SUBDIRS += QtXmlPatterns
        }
      }

    }

    !equals(HAVE_QT_XML, "0") {
      SUBDIRS += QtXml
    }

    !equals(HAVE_QT_SQL, "0") {
      SUBDIRS += QtSql
    }

    !equals(HAVE_QT_NETWORK, "0") {
      SUBDIRS += QtNetwork
    }

    !equals(HAVE_QT_DESIGNER, "0") {
      lessThan(QT_MAJOR_VERSION, 6) {
        SUBDIRS += QtDesigner
      }
    }

    !equals(HAVE_QT_UITOOLS, "0") {
      SUBDIRS += QtUiTools
    }

    greaterThan(QT_MAJOR_VERSION, 5) {
      !equals(HAVE_QT_CORE5COMPAT, "0") {
        SUBDIRS += QtCore5Compat
      }
    }

  }
}

ALL_DIRS = $$SUBDIRS

SUBDIRS += unit_tests
SUBDIRS += bridge_sample

unit_tests.depends += $$ALL_DIRS bridge_sample
