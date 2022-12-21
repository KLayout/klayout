
DESTDIR = $$OUT_PWD/..

include($$PWD/../klayout.pri)
include($$PWD/../with_all_libs.pri)

# NOTE: doc is needed for testing help sources

!equals(HAVE_QT, "0") {
  INCLUDEPATH += $$DOC_INC $$ICONS_INC
  DEPENDPATH += $$DOC_INC $$ICONS_INC

  LIBS += -lklayout_doc -lklayout_icons
}
  
TEMPLATE = app

# Don't build the ut_runner app as ordinary command line tool on MacOS
mac {
  CONFIG -= app_bundle
}

TARGET = ut_runner

SOURCES = \
  unit_test_main.cc \
  utTestConsole.cc \

HEADERS += \
  utTestConsole.h \

!win32 {
  LIBS += -ldl
} else {
  LIBS += -lshell32
}

LIBS += -lklayout_gsi_test

!equals(HAVE_QT, "0") {

  INCLUDEPATH += $$QTBASIC_INC
  DEPENDPATH += $$QTBASIC_INC

  equals(HAVE_QTBINDINGS, "1") {
    !equals(HAVE_QT_XML, "0") {
      LIBS += -lklayout_QtXml
    }
    greaterThan(QT_MAJOR_VERSION, 4) {
      LIBS += -lklayout_QtWidgets
    }
  }

} else {
  CONFIG -= qt
}

