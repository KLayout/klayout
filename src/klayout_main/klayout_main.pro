
DESTDIR = $$OUT_PWD/..

include($$PWD/../klayout.pri)

TARGET = klayout

include($$PWD/../app.pri)
include($$PWD/../with_all_libs.pri)

HEADERS = \

FORMS = \

SOURCES = \
  klayout.cc \

RESOURCES = \

win32 {

  windres.target = klayout_rc.o
  windres.depends = $$PWD/klayout.rc
  windres.commands = windres $$windres.depends $$windres.target

  PRE_TARGETDEPS += klayout_rc.o
  QMAKE_EXTRA_TARGETS += windres

  LIBS += $$windres.target

}
