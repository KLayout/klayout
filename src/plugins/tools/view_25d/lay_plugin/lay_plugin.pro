
TARGET = d25_ui
DESTDIR = $$OUT_PWD/../../../../lay_plugins

include($$PWD/../../../lay_plugin.pri)

INCLUDEPATH += $$RDB_INC $$ANT_INC $$QTBASIC_INC
DEPENDPATH += $$RDB_INC $$ANT_INC $$QTBASIC_INC

LIBS += -L$$DESTDIR/.. -lklayout_rdb -lklayout_ant
equals(HAVE_QTBINDINGS, "1") {
  LIBS += -lklayout_qtbasic -lklayout_QtGui -lklayout_QtCore
}

HEADERS = \
  layD25View.h \
  layD25ViewWidget.h \
    layD25MemChunks.h \
    layD25ViewUtils.h \
    layD25Camera.h

SOURCES = \
  gsiDeclLayD25View.cc \
  layD25View.cc \
  layD25ViewWidget.cc \
  layD25Plugin.cc \
    layD25MemChunks.cc \
    layD25ViewUtils.cc \
    layD25Camera.cc

FORMS = \
  D25View.ui \

RESOURCES = \
  layD25Resources.qrc \

greaterThan(QT_MAJOR_VERSION, 5) {
  QT += openglwidgets
}
