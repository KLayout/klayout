
TARGET = QtXml

include($$PWD/../pymod.pri)

SOURCES = \
  QtXmlMain.cc \

HEADERS += \

LIBS += -lklayout_QtCore -lklayout_QtXml

# Because of stupid dependency of QtCore on QtGui and this on QtWidgets:
LIBS += -lklayout_QtGui

greaterThan(QT_MAJOR_VERSION, 4) {
  LIBS += -lklayout_QtWidgets
}
