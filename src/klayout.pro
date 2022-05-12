
include(klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  tl \
  gsi \
  db \
  rdb \
  lib \
  plugins \
  unit_tests \
  buddies \
  lym \
  laybasic \
  ant \
  img \
  edt \

equals(HAVE_RUBY, "1") {
  SUBDIRS += drc lvs
}

!equals(HAVE_QT, "0") {

  # TODO: make buddies able to build without Qt
  SUBDIRS += \
    klayout_main \
    lay \
    fontgen \

}

LANG_DEPENDS =
MAIN_DEPENDS =

equals(HAVE_RUBY, "1") {
  SUBDIRS += rba
  LANG_DEPENDS += rba
  rba.depends += gsi db
} else {
  SUBDIRS += rbastub
  rbastub.depends += gsi
  LANG_DEPENDS += rbastub
}

equals(HAVE_PYTHON, "1") {
  SUBDIRS += pya
  LANG_DEPENDS += pya
  pya.depends += gsi db
  SUBDIRS += pymod
  pymod.depends += pya
} else {
  SUBDIRS += pyastub
  pyastub.depends += gsi
  LANG_DEPENDS += pyastub
}

gsi.depends += tl
db.depends += gsi
rdb.depends += db
lib.depends += db

buddies.depends += plugins lym $$LANG_DEPENDS
lym.depends += gsi $$LANG_DEPENDS

laybasic.depends += rdb 

ant.depends += laybasic
img.depends += laybasic
edt.depends += laybasic

plugins.depends += lib rdb db ant

equals(HAVE_RUBY, "1") {
  MAIN_DEPENDS += drc lvs
  drc.depends += rdb lym
  lvs.depends += drc
  buddies.depends += drc lvs
}

!equals(HAVE_QT, "0") {

  equals(HAVE_PYTHON, "1") {
    pymod.depends += lay
  }

  equals(HAVE_QTBINDINGS, "1") {

    SUBDIRS += gsiqt
    gsiqt.depends += gsi db
    laybasic.depends += gsiqt

    equals(HAVE_PYTHON, "1") {
      pymod.depends += gsiqt
    }

  }

  plugins.depends += lay

  lay.depends += laybasic ant img edt

  klayout_main.depends += plugins $$MAIN_DEPENDS

}

unit_tests.depends += plugins $$MAIN_DEPENDS $$LANG_DEPENDS
