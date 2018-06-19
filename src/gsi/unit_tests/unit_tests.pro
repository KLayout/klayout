
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = gsi_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  gsiExpression.cc \

HEADERS += \

INCLUDEPATH += $$TL_INC $$GSI_INC
DEPENDPATH += $$TL_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_tl -lklayout_gsi

