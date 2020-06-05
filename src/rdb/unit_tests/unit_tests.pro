
DESTDIR_UT = $$OUT_PWD/../..
DESTDIR = $$OUT_PWD/..

TARGET = rdb_tests

include($$PWD/../../lib_ut.pri)

SOURCES = \
  rdb.cc \
    rdbRVEReaderTests.cc

INCLUDEPATH += $$RDB_INC $$TL_INC $$DB_INC $$GSI_INC
DEPENDPATH += $$RDB_INC $$TL_INC $$DB_INC $$GSI_INC

LIBS += -L$$DESTDIR_UT -lklayout_rdb -lklayout_db -lklayout_tl -lklayout_gsi

