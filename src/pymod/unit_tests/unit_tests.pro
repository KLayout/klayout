
DESTDIR_UT = $$OUT_PWD/../..

TARGET = pymod_tests

include($$PWD/../../klayout.pri)
include($$PWD/../../lib_ut.pri)

SOURCES = \
  pymod_tests.cc

msvc {

  # "\\\\" is actually *one* backslash for replacement string and *two* backslashes in the
  # substitution string in qmake ... so we replace \ by \\ here:
  PYTHON_ESCAPED = $$replace(PYTHON, "\\\\", "\\\\")

  QMAKE_CXXFLAGS += \
    "-DPYTHON=\"$$PYTHON_ESCAPED\""

} else {

  DEFINES += \
    PYTHON=$$PYTHON \

}

INCLUDEPATH += $$DB_INC $$TL_INC $$GSI_INC
DEPENDPATH += $$DB_INC $$TL_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_db -lklayout_tl -lklayout_gsi
