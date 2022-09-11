
DESTDIR = $$OUT_PWD/../..

include($$PWD/../../klayout.pri)

TARGET = klayout

include($$PWD/../../app.pri)
include($$PWD/../../with_all_libs.pri)

HEADERS = \

FORMS = \

SOURCES = \
  klayout.cc \

RESOURCES = \

win32 {
  RC_FILE = $$PWD/klayout.rc
}

INCLUDEPATH += $$DOC_INC $$ICONS_INC $$QTBASIC_INC
DEPENDPATH += $$DOC_INC $$ICONS_INC $$QTBASIC_INC

LIBS += -lklayout_doc -lklayout_icons

equals(HAVE_QTBINDINGS, "1") {

  LIBS += -lklayout_qtbasic -lklayout_QtGui

  !equals(HAVE_QT_XML, "0") {
    LIBS += -lklayout_QtXml
  }
  !equals(HAVE_QT_NETWORK, "0") {
    LIBS += -lklayout_QtNetwork
  }
  !equals(HAVE_QT_SQL, "0") {
    LIBS += -lklayout_QtSql
  }
  !equals(HAVE_QT_DESIGNER, "0") {
    LIBS += -lklayout_QtDesigner
  }
  !equals(HAVE_QT_UITOOLS, "0") {
    LIBS += -lklayout_QtUiTools
  }

  greaterThan(QT_MAJOR_VERSION, 4) {

    LIBS += -lklayout_QtWidgets

    !equals(HAVE_QT_MULTIMEDIA, "0") {
      LIBS += -lklayout_QtMultimedia
    }
    !equals(HAVE_QT_PRINTSUPPORT, "0") {
      LIBS += -lklayout_QtPrintSupport
    }
    !equals(HAVE_QT_SVG, "0") {
      LIBS += -lklayout_QtSvg
    }
    !equals(HAVE_QT_XML, "0") {
      LIBS += -lklayout_QtXmlPatterns
    }

  }

  greaterThan(QT_MAJOR_VERSION, 5) {

    LIBS += -lklayout_QtCore5Compat
    LIBS -= -lklayout_QtXmlPatterns
    LIBS -= -lklayout_QtDesigner

  }

}
