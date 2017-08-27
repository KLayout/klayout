
DESTDIR = $$OUT_PWD/..

include($$PWD/../klayout.pri)
include($$PWD/../with_all_libs.pri)

TEMPLATE = app

TARGET = ut_runner

SOURCES = \
  unit_test_main.cc \

!win32 {
  LIBS += -ldl
}
