
include($$PWD/klayout.pri)

TEMPLATE = lib

inst_target.path = $$PREFIX
win32 {

  inst_target.files = $(DESTDIR_TARGET)

} else {

  inst_target.files = $$DESTDIR/$(TARGET)
  inst_target.extra = ln -s $(TARGET) $(TARGET0) && ln -s $(TARGET) $(TARGET1) && ln -s $(TARGET) $(TARGET2)

}

INSTALLS = inst_target
