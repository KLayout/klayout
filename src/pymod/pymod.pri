
DESTDIR = $$OUT_PWD/..
LIBDIR = $$OUT_PWD/../..
DESTDIR_PYMOD = $$DESTDIR/klayout

TEMPLATE = lib

include($$PWD/../klayout.pri)

INCLUDEPATH += $$PYTHONINCLUDE $$TL_INC $$GSI_INC $$PYA_INC
DEPENDPATH += $$PYTHONINCLUDE $$TL_INC $$GSI_INC $$PYA_INC
LIBS += $$PYTHONLIBFILE -L$$LIBDIR -lklayout_tl -lklayout_gsi -lklayout_pya

# Python is somewhat sloppy and relies on the compiler initializing fields 
# of strucs to 0:
QMAKE_CXXFLAGS_WARN_ON += \
    -Wno-missing-field-initializers

# Only on Windows, DESTDIR_TARGET is usable. On this platform, a blank happens to appear between
# $(DESTDIR) and $(TARGET)
win32 {

  QMAKE_POST_LINK += $(MKDIR) $$DESTDIR_PYMOD && $(COPY) $(DESTDIR_TARGET) $$DESTDIR_PYMOD/$${TARGET}$${PYTHONEXTSUFFIX}

  # to avoid the major version being appended to the dll name - in this case -lxyz won't link it again
  # because the library is called xyx0.dll.
  CONFIG += skip_target_version_ext

} else {

  QMAKE_POST_LINK += $(MKDIR) $$DESTDIR_PYMOD && $(COPY) $(DESTDIR)$(TARGET) $$DESTDIR_PYMOD/$${TARGET}$${PYTHONEXTSUFFIX}

}

lib_target.path = $$PREFIX/pymod/klayout
# This would be nice:
#   lib_target.files += $$DESTDIR_PYMOD/$${TARGET}$${PYTHONEXTSUFFIX}
# but some Qt versions need this explicitly:
lib_target.extra = $(INSTALL_PROGRAM) $$DESTDIR_PYMOD/$${TARGET}$${PYTHONEXTSUFFIX} $(INSTALLROOT)$$PREFIX/pymod/klayout
INSTALLS = lib_target

