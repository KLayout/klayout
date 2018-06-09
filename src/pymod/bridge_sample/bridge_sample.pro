
# The "bridge" library is a sample library illustrating how to access
# KLayout data from PyObject and vice versa.
#
# It demonstrates how to translate a db::SimplePolygon object to an
# array of x/y floating point pairs and back.
#
# To build this module externally you'll need to supply these variables in
# qmake:
#  - KLAYOUTINCLUDE: path to the KLayout sources
#  - KLAYOUTLIBDIR: path to the KLayout libraries (.so/.dll)
#  - PYTHONINCLUDE: path to the Python include file
#  - PYTHONLIBFILE: path to the Python library file
#
# Normally, these paths are set by the build script.
#
# To use the bridge library, ...

!isEmpty(KLAYOUTINCLUDE) {
  INC = $$KLAYOUTINCLUDE
} else {
  INC = $$PWD/../..
}

!isEmpty(KLAYOUTLIBDIR) {
  LIBDIR = $$KLAYOUTLIBDIR
} else {
  LIBDIR = $$OUT_PWD/../..
}

# That's were we are going to put our Python module to
DESTDIR = $$OUT_PWD/../..

# We're going to build a library
TEMPLATE = lib

# That's how we name our library
TARGET = bridge_sample

# The only source
SOURCES = \
    bridge_sample.cc

# Include QtCore required for some includes
QT = core

# Further dependencies include:
# - Python (of course)
# - GSI (generic scripting interface)
# - TL (basic toolkit)
# - PYA (Python binding for GSI)
INCLUDEPATH += $$PYTHONINCLUDE $$INC/tl/tl $$INC/gsi/gsi $$INC/pya/pya
DEPENDPATH += $$PYTHONINCLUDE $$INC/tl/tl $$INC/gsi/gsi $$INC/pya/pya
LIBS += $$PYTHONLIBFILE -L$$LIBDIR -lklayout_tl -lklayout_gsi -lklayout_pya

# Also include DB as this is our sample
INCLUDEPATH += $$INC/db/db
DEPENDPATH += $$INC/db/db
LIBS += -L$$LIBDIR -lklayout_db

# Pull in RPATH
!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH
}

# Some standard compiler warnings on
QMAKE_CXXFLAGS_WARN_ON += \
    -pedantic \
    -Woverloaded-virtual \
    -Wsign-promo \
    -Wsynth \
    -Wno-deprecated \
    -Wno-long-long \
    -Wno-strict-aliasing \
    -Wno-deprecated-declarations \
    -Wno-reserved-user-defined-literal \

# Python is somewhat sloppy and relies on the compiler initializing fields
# of strucs to 0:
QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-missing-field-initializers

win32 {
  # to avoid the major version being appended to the dll name - in this case -lxyz won't link it again
  # because the library is called xyx0.dll.
  CONFIG += skip_target_version_ext
} else {
  # Make the target library without the "lib" prefix on Linux
  QMAKE_POST_LINK += $(COPY) $(DESTDIR)$(TARGET) $$DESTDIR/$${TARGET}.so
}

# nothing to install as we're building a test library
INSTALLS =
