
DESTDIR = $$OUT_PWD/..
LIBDIR = $$OUT_PWD/../..
DESTDIR_PYMOD = $$DESTDIR/klayout

TEMPLATE = lib

include($$PWD/../klayout.pri)

INCLUDEPATH += "$$PYTHONINCLUDE" $$TL_INC $$GSI_INC $$PYA_INC
DEPENDPATH += "$$PYTHONINCLUDE" $$TL_INC $$GSI_INC $$PYA_INC
LIBS += "$$PYTHONLIBFILE" -L$$LIBDIR -lklayout_tl -lklayout_gsi -lklayout_pya

!msvc {
  # Python is somewhat sloppy and relies on the compiler initializing fields
  # of strucs to 0:
  QMAKE_CXXFLAGS_WARN_ON += \
      -Wno-missing-field-initializers
}

# Only on Windows, DESTDIR_TARGET is usable. On this platform, a blank happens to appear between
# $(DESTDIR) and $(TARGET)
win32 {

  msvc {
    QMAKE_POST_LINK += (if not exist $$shell_path($$DESTDIR_PYMOD) mkdir $$shell_path($$DESTDIR_PYMOD)) && $(COPY) $(DESTDIR_TARGET) $$shell_path($$DESTDIR_PYMOD/$${TARGET}$${PYTHONEXTSUFFIX})
  } else {
    QMAKE_POST_LINK += $(MKDIR) $$shell_path($$DESTDIR_PYMOD) && $(COPY) $(DESTDIR_TARGET) $$shell_path($$DESTDIR_PYMOD/$${TARGET}$${PYTHONEXTSUFFIX})
  }

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
msvc {
  lib_target.extra = $(INSTALL_PROGRAM) $$shell_path($$DESTDIR_PYMOD/$${TARGET}$${PYTHONEXTSUFFIX}) $$shell_path($(INSTALLROOT)$$PREFIX/pymod/klayout)
} else {
  lib_target.extra = $(INSTALL_PROGRAM) $$DESTDIR_PYMOD/$${TARGET}$${PYTHONEXTSUFFIX} $(INSTALLROOT)$$PREFIX/pymod/klayout
}
INSTALLS = lib_target

!equals(PYI, "") {

  msvc {
    QMAKE_POST_LINK += && $(COPY) $$shell_path($$PWD/distutils_src/klayout/$$PYI) $$shell_path($$DESTDIR_PYMOD)
  } else {
    QMAKE_POST_LINK += && $(MKDIR) $$DESTDIR_PYMOD && $(COPY) $$PWD/distutils_src/klayout/$$PYI $$DESTDIR_PYMOD
  }

  POST_TARGETDEPS += $$PWD/distutils_src/klayout/$$PYI

  # INSTALLS needs to be inside a lib or app templates.
  modpyi_target.path = $$PREFIX/pymod/klayout
  # This would be nice:
  #   init_target.files += $$DESTDIR_PYMOD/$$REALMODULE/*
  # but some Qt versions need this explicitly:
  msvc {
    modpyi_target.extra = $(INSTALL_PROGRAM) $$shell_path($$DESTDIR_PYMOD/$$PYI) $$shell_path($(INSTALLROOT)$$PREFIX/pymod/klayout)
  } else {
    modpyi_target.extra = $(INSTALL_PROGRAM) $$DESTDIR_PYMOD/$$PYI $(INSTALLROOT)$$PREFIX/pymod/klayout
  }

  # Not yet. As long as .pyi files are not generated automatically,
  # this does not make much sense:
  # INSTALLS += modpyi_target

}

!equals(REALMODULE, "") {

  msvc {
    QMAKE_POST_LINK += && (if not exist $$shell_path($$DESTDIR_PYMOD/$$REALMODULE) $(MKDIR) $$shell_path($$DESTDIR_PYMOD/$$REALMODULE)) && $(COPY) $$shell_path($$PWD/distutils_src/klayout/$$REALMODULE/*.py) $$shell_path($$DESTDIR_PYMOD/$$REALMODULE)
  } else {
    QMAKE_POST_LINK += && $(MKDIR) $$DESTDIR_PYMOD/$$REALMODULE && $(COPY) $$PWD/distutils_src/klayout/$$REALMODULE/*.py $$DESTDIR_PYMOD/$$REALMODULE
  }

  # INSTALLS needs to be inside a lib or app templates.
  modsrc_target.path = $$PREFIX/pymod/klayout/$$REALMODULE
  # This would be nice:
  #   init_target.files += $$DESTDIR_PYMOD/$$REALMODULE/*
  # but some Qt versions need this explicitly:
  msvc {
    modsrc_target.extra = $(INSTALL_PROGRAM) $$shell_path($$DESTDIR_PYMOD/$$REALMODULE/*.py) $$shell_path($(INSTALLROOT)$$PREFIX/pymod/klayout/$$REALMODULE)
  } else {
    modsrc_target.extra = $(INSTALL_PROGRAM) $$DESTDIR_PYMOD/$$REALMODULE/*.py $(INSTALLROOT)$$PREFIX/pymod/klayout/$$REALMODULE
  }
  INSTALLS += modsrc_target

}
