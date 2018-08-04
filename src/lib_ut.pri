
include($$PWD/klayout.pri)

TEMPLATE = lib

# Only on Windows, DESTDIR_TARGET is usable. On this platform, a blank happens to appear between
# $(DESTDIR) and $(TARGET)
win32 {

  QMAKE_POST_LINK += $(COPY) $(DESTDIR_TARGET) $$shell_path($$DESTDIR_UT/$${TARGET}.ut)

  # to avoid the major version being appended to the dll name - in this case -lxyz won't link it again
  # because the library is called xyx0.dll.
  CONFIG += skip_target_version_ext

} else {
  QMAKE_POST_LINK += $(COPY) $(DESTDIR)$(TARGET) $$DESTDIR_UT/$${TARGET}.ut
}

equals(HAVE_QT5, "1") {
  QT += testlib
} else {
  CONFIG += qtestlib
}
