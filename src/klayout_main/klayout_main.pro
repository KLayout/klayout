
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
  RC_FILE = $$PWD/klayout.rc
}
