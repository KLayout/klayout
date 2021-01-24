
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
DESTDIR = $$OUT_PWD/..

# We're going to build a library
TEMPLATE = lib

# That's how we name our library
TARGET = bridge_mod

# The only source
SOURCES = \
    bridge_sample.cc

# Depending on whether we have a Qt-less build or not we
# need to include QtCore. Also set -DHAVE_QT
equals(HAVE_QT, "0") {
  QT =
} else {
  # Include QtCore required for some includes
  QT = core
  DEFINES += HAVE_QT
}

# Further dependencies include:
# - Python (of course)
# - GSI (generic scripting interface)
# - TL (basic toolkit)
# - PYA (Python binding for GSI)
!isEmpty(BITS_PATH) {
  # pull the definitions from the BITS_PATH if set
  include($$BITS_PATH/python/python.pri)
}
INCLUDEPATH += "$$PYTHONINCLUDE" $$INC/tl/tl $$INC/pya/pya $$INC/gsi/gsi
DEPENDPATH += "$$PYTHONINCLUDE" $$INC/tl/tl $$INC/pya/pya $$INC/gsi/gsi
LIBS += "$$PYTHONLIBFILE" -L$$LIBDIR -lklayout_tl -lklayout_pya -lklayout_gsi

# Also include DB as this is our sample
INCLUDEPATH += $$INC/db/db
DEPENDPATH += $$INC/db/db
LIBS += -L$$LIBDIR -lklayout_db

# Pull in RPATH
!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH
}

!msvc {

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

  # because we use unordered_map/unordered_set:
  QMAKE_CXXFLAGS += -std=c++0x

  # Python is somewhat sloppy and relies on the compiler initializing fields
  # of strucs to 0:
  QMAKE_CXXFLAGS_WARN_ON += \
      -Wno-missing-field-initializers

} else {

  # disable some warnings
  QMAKE_CXXFLAGS += \
      /wd4251 \         # DLL interface required

}

win32 {
  # to avoid the major version being appended to the dll name - in this case -lxyz won't link it again
  # because the library is called xyx0.dll.
  CONFIG += skip_target_version_ext
  # make the proper library name for Python
  QMAKE_POST_LINK += $(COPY) $(DESTDIR_TARGET) $$shell_path($$DESTDIR/$${TARGET}$${PYTHONEXTSUFFIX})
} else {
  # Make the target library without the "lib" prefix on Linux
  QMAKE_POST_LINK += $(COPY) $(DESTDIR)$(TARGET) $$DESTDIR/$${TARGET}$${PYTHONEXTSUFFIX}
}

# nothing to install as we're building a test library
INSTALLS =
