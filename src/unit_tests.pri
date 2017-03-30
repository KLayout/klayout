
INCLUDEPATH += $$PYTHONINCLUDE $$PWD/tl $$PWD/gsi $$PWD/db $$PWD/rdb $$PWD/laybasic $$PWD/lay $$PWD/ant $$PWD/img $$PWD/edt $$PWD/ext $$PWD/lib $$PWD/common $$PWD/ut
DEPENDPATH += $$PYTHONINCLUDE $$PWD/tl $$PWD/gsi $$PWD/db $$PWD/rdb $$PWD/laybasic $$PWD/lay $$PWD/ant $$PWD/img $$PWD/edt $$PWD/ext $$PWD/lib $$PWD/commo $$PWD/ut

LIBS += $$PYTHONLIBFILE $$RUBYLIBFILE -L$$DESTDIR -lklayout_tl -lklayout_gsi -lklayout_db -lklayout_rdb -lklayout_laybasic -lklayout_lay -lklayout_ant -lklayout_img -lklayout_edt -lklayout_ext -lklayout_lib -lklayout_ut

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic $$DESTDIR/lay $$DESTDIR/ext
DEPENDPATH += $$DESTDIR/laybasic $$DESTDIR/lay $$DESTDIR/ext

equals(HAVE_QTBINDINGS, "1") {
  INCLUDEPATH += $$PWD/gsiqt
  DEPENDPATH += $$PWD/gsiqt
  LIBS += -lklayout_gsiqt
}

equals(HAVE_RUBY, "1") {
  INCLUDEPATH += $$PWD/rba
  DEPENDPATH += $$PWD/rba
  LIBS += -lklayout_rba
} else {
  INCLUDEPATH += $$PWD/rbastub
  DEPENDPATH += $$PWD/rbastub
  LIBS += -lklayout_rbastub
}

equals(HAVE_PYTHON, "1") {
  INCLUDEPATH += $$PWD/pya
  DEPENDPATH += $$PWD/pya
  LIBS += -lklayout_pya
} else {
  INCLUDEPATH += $$PWD/pyastub
  DEPENDPATH += $$PWD/pyastub
  LIBS += -lklayout_pyastub
}
