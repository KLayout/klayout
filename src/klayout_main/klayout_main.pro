
DESTDIR = $$OUT_PWD/..

include($$PWD/../klayout.pri)

TEMPLATE = app

TARGET = klayout

HEADERS = \

FORMS = \

SOURCES = \
  klayout.cc \

RESOURCES = \

INCLUDEPATH += ../tl ../gsi ../db ../rdb ../laybasic ../lay ../ext ../img ../ant ../lib ../version
DEPENDPATH += ../tl ../gsi ../db ../rdb ../laybasic ../lay ../ext ../img ../ant ../lib ../version
LIBS += $$PYTHONLIBFILE $$RUBYLIBFILE -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb -lklayout_laybasic -lklayout_lay -lklayout_ant -lklayout_img -lklayout_edt -lklayout_ext -lklayout_lib

win32 {

  windres.target = klayout_rc.o
  windres.depends = $$PWD/klayout.rc
  windres.commands = windres $$windres.depends $$windres.target

  PRE_TARGETDEPS += klayout_rc.o
  QMAKE_EXTRA_TARGETS += windres

  LIBS += $$windres.target

}

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic $$OUT_PWD/../lay
DEPENDPATH += $$DESTDIR/laybasic $$OUT_PWD/../lay

equals(HAVE_QTBINDINGS, "1") {
  INCLUDEPATH += ../gsiqt
  DEPENDPATH += ../gsiqt
  LIBS += -lklayout_gsiqt
}

equals(HAVE_RUBY, "1") {
  INCLUDEPATH += ../rba
  DEPENDPATH += ../rba
  LIBS += -lklayout_rba
} else {
  INCLUDEPATH += ../rbastub
  DEPENDPATH += ../rbastub
  LIBS += -lklayout_rbastub
}

equals(HAVE_PYTHON, "1") {
  INCLUDEPATH += ../pya
  DEPENDPATH += ../pya
  LIBS += -lklayout_pya
} else {
  INCLUDEPATH += ../pyastub
  DEPENDPATH += ../pyastub
  LIBS += -lklayout_pyastub
}
