
TEMPLATE = app

inst_target.path = $$PREFIX
win32 {
  inst_target.files = $$DESTDIR/$${TARGET}.exe
} else {
  inst_target.files = $$DESTDIR/$${TARGET}
}
INSTALLS = inst_target
