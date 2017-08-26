
include($$PWD/klayout.pri)

TEMPLATE = lib

# Only on Windows, DESTDIR_TARGET is usable. On this platform, a blank happens to appear between
# $(DESTDIR) and $(TARGET)
win32 {
  QMAKE_POST_LINK += $(COPY) $(DESTDIR_TARGET) $$DESTDIR_UT/$${TARGET}.ut
} else {
  QMAKE_POST_LINK += $(COPY) $(DESTDIR)$(TARGET) $$DESTDIR_UT/$${TARGET}.ut
}

equals(HAVE_QT5, "1") {
  QT += testlib
} else {
  CONFIG += qtestlib
}
