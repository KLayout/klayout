
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

GSIQT_INC = $$PWD/gsiqt

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

equals(HAVE_RUBY, "1") {
  DEFINES += \
    HAVE_RUBY \
    HAVE_RUBY_VERSION_CODE=$$RUBYVERSIONCODE 
}

QMAKE_RPATHDIR += $$PREFIX

QMAKE_CXXFLAGS_WARN_ON += \
    -pedantic \
    -Woverloaded-virtual \
    -Wsign-promo \
    -Wsynth \
    -Wno-deprecated \
    -Wno-long-long \
    -Wno-strict-aliasing \
    -Wno-deprecated-declarations \

QT += network xml sql 

equals(HAVE_QT5, "1") {
  QT += designer printsupport
  equals(HAVE_QTBINDINGS, "1") {
    QT += multimedia multimediawidgets xmlpatterns svg gui
  }
} else {
  # questionable: use uitools instead?
  CONFIG += designer 
}

# only support the required symbols for shared object load performance
win32 {
  QMAKE_LFLAGS += -Wl,--exclude-all-symbols
} else {
  QMAKE_CXXFLAGS += -fvisibility=hidden
}

DEFINES += \
  KLAYOUT_VERSION=$$KLAYOUT_VERSION \
  KLAYOUT_VERSION_REV=$$KLAYOUT_VERSION_REV \
  KLAYOUT_VERSION_DATE=$$KLAYOUT_VERSION_DATE \

VERSION = $$KLAYOUT_VERSION
