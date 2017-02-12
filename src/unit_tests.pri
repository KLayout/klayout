
INCLUDEPATH += $$PYTHONINCLUDE $$PWD/tl $$PWD/gsi $$PWD/db $$PWD/rdb $$PWD/laybasic $$PWD/lay $$PWD/ant $$PWD/img $$PWD/edt $$PWD/ext $$PWD/lib $$PWD/common $$PWD/ut
DEPENDPATH += $$PYTHONINCLUDE $$PWD/tl $$PWD/gsi $$PWD/db $$PWD/rdb $$PWD/laybasic $$PWD/lay $$PWD/ant $$PWD/img $$PWD/edt $$PWD/ext $$PWD/lib $$PWD/commo $$PWD/ut

LIBS += $$PYTHONLIBFILE $$RUBYLIBFILE -L$$DESTDIR -ltl -lgsi -ldb -lrdb -llaybasic -llay -lant -limg -ledt -lext -llib -lut

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic $$DESTDIR/lay $$DESTDIR/ext
DEPENDPATH += $$DESTDIR/laybasic $$DESTDIR/lay $$DESTDIR/ext

equals(HAVE_QTBINDINGS, "1") {
  equals(HAVE_QT5, "1") {
    INCLUDEPATH += $$PWD/gsiqt5
    DEPENDPATH += $$PWD/gsiqt5
    LIBS += -lgsiqt5 
  } else {
    INCLUDEPATH += $$PWD/gsiqt
    DEPENDPATH += $$PWD/gsiqt
    LIBS += -lgsiqt 
  }
}

equals(HAVE_RUBY, "1") {
  INCLUDEPATH += $$PWD/rba
  DEPENDPATH += $$PWD/rba
  LIBS += -lrba
} else {
  INCLUDEPATH += $$PWD/rbastub
  DEPENDPATH += $$PWD/rbastub
  LIBS += -lrbastub
}

equals(HAVE_PYTHON, "1") {
  INCLUDEPATH += $$PWD/pya
  DEPENDPATH += $$PWD/pya
  LIBS += -lpya
} else {
  INCLUDEPATH += $$PWD/pyastub
  DEPENDPATH += $$PWD/pyastub
  LIBS += -lpyastub
}
