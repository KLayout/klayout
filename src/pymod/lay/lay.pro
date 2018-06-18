
TARGET = lay

include($$PWD/../pymod.pri)

SOURCES = \
  layMain.cc \

HEADERS += \

LIBS += -lklayout_lay

# Use this opportunity to provide the __init__.py file

equals(HAVE_QTBINDINGS, "1") {
  equals(HAVE_QT5, "1") {
    INIT_PY = $$PWD/../__init__.py.qt5
  } else {
    INIT_PY = $$PWD/../__init__.py.qt4
  }
} else {
  INIT_PY = $$PWD/../__init__.py.noqt
}

QMAKE_POST_LINK += && $(COPY) $$INIT_PY $$DESTDIR_PYMOD/__init__.py

# INSTALLS needs to be inside a lib or app templates.
init_target.path = $$PREFIX/pymod/klayout
init_target.files += $$DESTDIR_PYMOD/__init__.py
INSTALLS += init_target
