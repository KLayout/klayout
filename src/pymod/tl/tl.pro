
TARGET = tl

include($$PWD/../pymod.pri)

SOURCES = \
  tlMain.cc \

HEADERS += \

# Use this opportunity to provide the __init__.py file

equals(HAVE_QTBINDINGS, "1") {
  equals(HAVE_QT5, "1") {
    INIT_PY = $$PWD/../__init__.py.qt5
  } else {
    INIT_PY = $$PWD/../__init__.py.qt4
  }
} else {
  equals(HAVE_QT, "0") {
    INIT_PY = $$PWD/../__init__.py.qtless
  } else {
    INIT_PY = $$PWD/../__init__.py.noqt
  }
}

msvc {
  QMAKE_POST_LINK += && $(COPY) $$shell_path($$INIT_PY) $$shell_path($$DESTDIR_PYMOD/__init__.py)
} else {
  QMAKE_POST_LINK += && $(COPY) $$INIT_PY $$DESTDIR_PYMOD/__init__.py
}

# INSTALLS needs to be inside a lib or app templates.
init_target.path = $$PREFIX/pymod/klayout
# This would be nice:
#   init_target.files += $$DESTDIR_PYMOD/__init__.py
# but some Qt versions need this explicitly:
msvc {
  init_target.extra = $(INSTALL_PROGRAM) $$shell_path($$DESTDIR_PYMOD/__init__.py) $$shell_path($(INSTALLROOT)$$PREFIX/pymod/klayout)
} else {
  init_target.extra = $(INSTALL_PROGRAM) $$DESTDIR_PYMOD/__init__.py $(INSTALLROOT)$$PREFIX/pymod/klayout
}
INSTALLS += init_target
