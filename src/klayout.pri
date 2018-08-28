
TL_INC = $$PWD/tl/tl
DB_INC = $$PWD/db/db
DRC_INC = $$PWD/drc/drc
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
} else {
  RBA_INC = $$PWD/rbastub
}

equals(HAVE_PYTHON, "1") {
  PYA_INC = $$PWD/pya/pya
} else {
  PYA_INC = $$PWD/pyastub
}

equals(HAVE_QTBINDINGS, "1") {
  DEFINES += HAVE_QTBINDINGS
}

equals(HAVE_64BIT_COORD, "1") {
  DEFINES += HAVE_64BIT_COORD
}

equals(HAVE_PYTHON, "1") {
  DEFINES += HAVE_PYTHON
}

equals(HAVE_CURL, "1") {
  DEFINES += HAVE_CURL
}

equals(HAVE_EXPAT, "1") {
  DEFINES += HAVE_EXPAT
}

equals(HAVE_RUBY, "1") {
  DEFINES += \
    HAVE_RUBY \
    HAVE_RUBY_VERSION_CODE=$$RUBYVERSIONCODE 
}

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH
}

msvc {

  INCLUDEPATH += \
    $$THIRD_PARTY/zlib/1.2.11/include \

  QMAKE_CXXFLAGS += -bigobj

  QMAKE_CXXFLAGS_WARN_ON += \

}

!msvc {

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

} else {

  # disable some warnings
  QMAKE_CXXFLAGS += \
      /wd4251 \         # DLL interface required

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

# only support the required symbols for shared object load performance
msvc {
  # ...
} else {
  win32 {
    QMAKE_LFLAGS += -Wl,--exclude-all-symbols
  } else {
    QMAKE_CXXFLAGS += -fvisibility=hidden
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
