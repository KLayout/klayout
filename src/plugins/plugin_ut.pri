
DESTDIR_KLP = $$OUT_PWD/../../..
DESTDIR = $$OUT_PWD/..

TEMPLATE = lib

INCLUDEPATH += ../src ../../../db ../../../tl ../../../gsi ../../../laybasic ../../../lay ../../../common ../../../ut
DEPENDPATH += ../src ../../../db ../../../tl ../../../gsi ../../../laybasic ../../../lay ../../../common ../../../ut
LIBS += -L$$DESTDIR_KLP -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_lay -lklayout_ut

# Only on Windows, DESTDIR_TARGET is usable. On this platform, a blank happens to appear between
# $(DESTDIR) and $(TARGET)
win32 {
  QMAKE_POST_LINK += $(COPY) $(DESTDIR_TARGET) $$DESTDIR_KLP/$${TARGET}.klp_ut
} else {
  QMAKE_POST_LINK += $(COPY) $(DESTDIR)$(TARGET) $$DESTDIR_KLP/$${TARGET}.klp_ut
}
