
DESTDIR = $$OUT_PWD/..

include($$PWD/../klayout.pri)
include($$PWD/../with_all_libs.pri)

TEMPLATE = app

# Don't build the ut_runner app as ordinary command line tool on MacOS
mac {
  CONFIG -= app_bundle
}

TARGET = ut_runner

SOURCES = \
  unit_test_main.cc \
  utTestConsole.cc \

HEADERS += \
  utTestConsole.h \

!win32 {
  LIBS += -ldl
}
