
TEMPLATE = lib

inst_target.path = $$PREFIX
win32 {
  inst_target.files = $$DESTDIR/$${TARGET}.dll
} else {
  inst_target.files = $$DESTDIR/lib$${TARGET}.so
}
INSTALLS = inst_target
