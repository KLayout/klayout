
TARGET = pyacore
REALMODULE = pya

include($$PWD/../pymod.pri)

SOURCES = \
  pyaMain.cc \

HEADERS += \

# needs all libraries as dependencies because we register all
# classes

LIBS += -lklayout_layview
  
!equals(HAVE_QT, "0") {
  LIBS += -lklayout_layui
  LIBS += -lklayout_lay
}

LIBS += -lklayout_laybasic 
LIBS += -lklayout_img 
LIBS += -lklayout_lib 
LIBS += -lklayout_edt 
LIBS += -lklayout_ant 
LIBS += -lklayout_lym
LIBS += -lklayout_db 
LIBS += -lklayout_rdb

# adds all the Qt binding stuff
!equals(HAVE_QT, "0") {

  equals(HAVE_QTBINDINGS, "1") {

    LIBS += -lklayout_QtCore -lklayout_QtGui

    !equals(HAVE_QT_NETWORK, "0") {
      LIBS += -lklayout_QtNetwork
      DEFINES += INCLUDE_QTNETWORK
    }

    greaterThan(QT_MAJOR_VERSION, 4) {
      LIBS += -lklayout_QtWidgets
      DEFINES += INCLUDE_QTWIDGETS
    }

    !equals(HAVE_QT_MULTIMEDIA, "0") {
      greaterThan(QT_MAJOR_VERSION, 4) {
        LIBS += -lklayout_QtMultimedia
        DEFINES += INCLUDE_QTMULTIMEDIA
      }
    }

    !equals(HAVE_QT_PRINTSUPPORT, "0") {
      greaterThan(QT_MAJOR_VERSION, 4) {
        LIBS += -lklayout_QtPrintSupport
        DEFINES += INCLUDE_QTPRINTSUPPORT
      }
    }

    !equals(HAVE_QT_SVG, "0") {
      greaterThan(QT_MAJOR_VERSION, 4) {
        LIBS += -lklayout_QtSvg
        DEFINES += INCLUDE_QTSVG
      }
    }

    !equals(HAVE_QT_XML, "0") {
      LIBS += -lklayout_QtXml
      DEFINES += INCLUDE_QTXML
      greaterThan(QT_MAJOR_VERSION, 4) {
        lessThan(QT_MAJOR_VERSION, 6) {
          LIBS += -lklayout_QtXmlPatterns
          DEFINES += INCLUDE_QTXMLPATTERNS
        }
      }
    }

    !equals(HAVE_QT_SQL, "0") {
      LIBS += -lklayout_QtSql
      DEFINES += INCLUDE_QTSQL
    }

    !equals(HAVE_QT_DESIGNER, "0") {
      lessThan(QT_MAJOR_VERSION, 6) {
        LIBS += -lklayout_QtDesigner
        DEFINES += INCLUDE_QTDESIGNER
      }
    }

    !equals(HAVE_QT_UITOOLS, "0") {
      LIBS += -lklayout_QtUiTools
      DEFINES += INCLUDE_QTUITOOLS
    }

    !equals(HAVE_QT_CORE5COMPAT, "0") {
      greaterThan(QT_MAJOR_VERSION, 5) {
        LIBS += -lklayout_QtCore5Compat
        DEFINES += INCLUDE_QTCORE5COMPAT
      }
    }

  }

}
