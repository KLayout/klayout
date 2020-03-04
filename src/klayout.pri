
TL_INC = $$PWD/tl/tl
DB_INC = $$PWD/db/db
DRC_INC = $$PWD/drc/drc
LVS_INC = $$PWD/lvs/lvs
EDT_INC = $$PWD/edt/edt
EXT_INC = $$PWD/ext/ext
GSI_INC = $$PWD/gsi/gsi
ANT_INC = $$PWD/ant/ant
RDB_INC = $$PWD/rdb/rdb
IMG_INC = $$PWD/img/img
LYM_INC = $$PWD/lym/lym
LIB_INC = $$PWD/lib/lib
LAY_INC = $$PWD/lay/lay
LAYBASIC_INC = $$PWD/laybasic/laybasic

QTBASIC_INC = $$PWD/gsiqt/qtbasic

BD_INC = $$PWD/buddies/src/bd

VERSION_INC = $$PWD/version

equals(HAVE_RUBY, "1") {
  RBA_INC = $$PWD/rba/rba
  RBA_LIB = klayout_rba
} else {
  RBA_INC = $$PWD/rbastub
  RBA_LIB = klayout_rbastub
}

equals(HAVE_PYTHON, "1") {
  PYA_INC = $$PWD/pya/pya
  PYA_LIB = klayout_pya
} else {
  PYA_INC = $$PWD/pyastub
  PYA_LIB = klayout_pyastub
}

equals(HAVE_QTBINDINGS, "1") {
  DEFINES += HAVE_QTBINDINGS
}

equals(HAVE_64BIT_COORD, "1") {
  DEFINES += HAVE_64BIT_COORD
}

equals(HAVE_PYTHON, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/python/python.pri)
  }
  DEFINES += HAVE_PYTHON
}

equals(HAVE_CURL, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/curl/curl.pri)
  } else {
    LIBS += -lcurl
    win32 {
      LIBS += -lwsock32    # required because we do "select"
    }
  }
  DEFINES += HAVE_CURL
}

equals(HAVE_EXPAT, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/expat/expat.pri)
  } else {
    LIBS += -lexpat
  }
  DEFINES += HAVE_EXPAT
}

equals(HAVE_PTHREADS, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/ptw/ptw.pri)
  } else {
    LIBS += -lpthread
  }
  DEFINES += HAVE_PTHREADS
}

equals(HAVE_RUBY, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/ruby/ruby.pri)
  }
  DEFINES += \
    HAVE_RUBY \
    HAVE_RUBY_VERSION_CODE=$$RUBYVERSIONCODE 
}

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH
}

!isEmpty(BITS_PATH) {
  include($$BITS_PATH/zlib/zlib.pri)
} else {
  !msvc {
    LIBS += -lz
  }
}

equals(HAVE_CRONOLOGY, "1") {
  DEFINES += HAVE_CRONOLOGY
  LIBS += $$CRONOLOGY_LIB
  INCLUDEPATH += $$CRONOLOGY_INCLUDE
}

msvc {

  QMAKE_CXXFLAGS += \
      /bigobj \
      /wd4251 \                   # Disable "DLL interface required"
      /source-charset:utf-8 \     # Use UTF-8 for source files

  QMAKE_LFLAGS += \
      /OPT:NOREF \                # preserves functions we want to keep for implementing the "force link" mechanism

  QMAKE_CXXFLAGS_WARN_ON += \

  # as we're using default-constructed iterators as "null" we can't have
  # checked iterators with MSVC
  DEFINES += _ITERATOR_DEBUG_LEVEL=0

} else {

  CONFIG(gcov) {
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
    QMAKE_LFLAGS += --coverage
  }

  QMAKE_CXXFLAGS_WARN_ON += \
      -pedantic \
      -Woverloaded-virtual \
      -Wsign-promo \
      -Wsynth \
      -Wno-deprecated \
      -Wno-long-long \
      -Wno-strict-aliasing \
      -Wno-deprecated-declarations \
      -Wno-reserved-user-defined-literal \

  # because we use unordered_map/unordered_set:
  QMAKE_CXXFLAGS += -std=c++0x

  win32 {
    QMAKE_LFLAGS += -Wl,--exclude-all-symbols
  } else {
    QMAKE_CXXFLAGS += -fvisibility=hidden
  }

}

equals(HAVE_QT, "0") {

  QT =

} else {

  DEFINES += HAVE_QT
  QT += core network xml sql

  equals(HAVE_QT5, "1") {
    QT += designer printsupport
    equals(HAVE_QTBINDINGS, "1") {
      QT += multimedia multimediawidgets xmlpatterns svg gui
    }
  } else {
    # questionable: use uitools instead?
    CONFIG += designer
  }

}

VERSION_STRING = $$KLAYOUT_VERSION
VERSION_STRING_WITH_BLANKS = $$replace(VERSION_STRING, "\\.", " ")
eval(VERSION_ARRAY = $$VERSION_STRING_WITH_BLANKS)

KLAYOUT_MAJOR_VERSION = $$member(VERSION_ARRAY, 0)

KLAYOUT_MINOR_VERSION = $$member(VERSION_ARRAY, 1)

KLAYOUT_TINY_VERSION = $$member(VERSION_ARRAY, 2)
isEmpty(KLAYOUT_TINY_VERSION) {
  KLAYOUT_TINY_VERSION = 0
}

DEFINES += \
  KLAYOUT_VERSION=$$KLAYOUT_VERSION \
  KLAYOUT_VERSION_REV=$$KLAYOUT_VERSION_REV \
  KLAYOUT_VERSION_DATE=$$KLAYOUT_VERSION_DATE \
  KLAYOUT_MAJOR_VERSION=$$KLAYOUT_MAJOR_VERSION \
  KLAYOUT_MINOR_VERSION=$$KLAYOUT_MINOR_VERSION \
  KLAYOUT_TINY_VERSION=$$KLAYOUT_TINY_VERSION \

VERSION = $$KLAYOUT_VERSION
