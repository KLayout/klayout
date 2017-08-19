
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TEMPLATE = lib

INCLUDEPATH += ../src ../../db ../../tl ../../gsi ../../laybasic ../../lay ../../ut
DEPENDPATH += ../src ../../db ../../tl ../../gsi ../../laybasic ../../lay ../../ut
LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi -lklayout_laybasic -lklayout_lay -lklayout_ut

QMAKE_RPATHDIR += $$DESTDIR

# Only on Windows, DESTDIR_TARGET is usable. On this platform, a blank happens to appear between
# $(DESTDIR) and $(TARGET)
win32 {
  QMAKE_POST_LINK += $(COPY) $(DESTDIR_TARGET) $$DESTDIR_UT/$${TARGET}.ut
} else {
  QMAKE_POST_LINK += $(COPY) $(DESTDIR)$(TARGET) $$DESTDIR_UT/$${TARGET}.ut
}
