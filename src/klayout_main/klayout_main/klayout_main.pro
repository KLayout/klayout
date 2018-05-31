
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

INCLUDEPATH += $$QTBASIC_INC
DEPENDPATH += $$QTBASIC_INC

equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lklayout_qtbasic -lklayout_QtGui -lklayout_QtXml -lklayout_QtNetwork -lklayout_QtSql -lklayout_QtDesigner
  equals(HAVE_QT5, "1") {
    LIBS += -lklayout_QtMultimedia -lklayout_QtPrintSupport -lklayout_QtSvg -lklayout_QtWidgets -lklayout_QtXmlPatterns
  }
}
