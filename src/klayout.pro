
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

!equals(HAVE_QT, "0") {

  # TODO: make buddies able to build without Qt
  SUBDIRS += \
    klayout_main \
    laybasic \
    lay \
    ant \
    buddies \
    lym \
    img \
    edt \
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

plugins.depends += lib rdb db

!equals(HAVE_QT, "0") {

  buddies.depends += plugins lym $$LANG_DEPENDS

  equals(HAVE_PYTHON, "1") {
    pymod.depends += lay
  }

  equals(HAVE_RUBY, "1") {
    SUBDIRS += drc lvs
    MAIN_DEPENDS += drc lvs
    drc.depends += rdb lym
    lvs.depends += drc
  }

  equals(HAVE_QTBINDINGS, "1") {

    SUBDIRS += gsiqt
    gsiqt.depends += gsi db
    laybasic.depends += gsiqt

    equals(HAVE_PYTHON, "1") {
      pymod.depends += gsiqt
    }

  }

  plugins.depends += lay ant

  lym.depends += gsi $$LANG_DEPENDS
  laybasic.depends += rdb lym
  ant.depends += laybasic
  img.depends += laybasic
  edt.depends += laybasic
  lay.depends += laybasic ant img edt
  klayout_main.depends += plugins $$MAIN_DEPENDS

}

unit_tests.depends += plugins $$MAIN_DEPENDS $$LANG_DEPENDS

RESOURCES += \
    plugins/tools/import/lay_plugin/layResources.qrc \
    laybasic/laybasic/layResources.qrc

