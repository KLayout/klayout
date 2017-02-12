
DESTDIR=$$OUT_PWD/..

include($$PWD/../klayout.pri)

TEMPLATE = app

TARGET = klayout

HEADERS = \

FORMS = \

SOURCES = \
  klayout.cc \

RESOURCES = \

INCLUDEPATH += ../tl ../gsi ../db ../rdb ../laybasic ../lay ../ext ../img ../ant ../lib
DEPENDPATH += ../tl ../gsi ../db ../rdb ../laybasic ../lay ../ext ../img ../ant ../lib
LIBS += $$PYTHONLIBFILE $$RUBYLIBFILE -L$$DESTDIR -ltl -lgsi -ldb -lrdb -llaybasic -llay -lant -limg -ledt -lext -llib

# Note: this accounts for UI-generated headers placed into the output folders in
# shadow builds:
INCLUDEPATH += $$DESTDIR/laybasic $$OUT_PWD/../lay
DEPENDPATH += $$DESTDIR/laybasic $$OUT_PWD/../lay

equals(HAVE_QTBINDINGS, "1") {
  equals(HAVE_QT5, "1") {
    INCLUDEPATH += ../gsiqt5
    DEPENDPATH += ../gsiqt5
    LIBS += -lgsiqt5 
  } else {
    INCLUDEPATH += ../gsiqt
    DEPENDPATH += ../gsiqt
    LIBS += -lgsiqt 
  }
}

equals(HAVE_RUBY, "1") {
  INCLUDEPATH += ../rba
  DEPENDPATH += ../rba
  LIBS += -lrba 
} else {
  INCLUDEPATH += ../rbastub
  DEPENDPATH += ../rbastub
  LIBS += -lrbastub 
}

equals(HAVE_PYTHON, "1") {
  INCLUDEPATH += ../pya
  DEPENDPATH += ../pya
  LIBS += -lpya 
} else {
  INCLUDEPATH += ../pyastub
  DEPENDPATH += ../pyastub
  LIBS += -lpyastub
}
