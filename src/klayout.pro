
include(klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  tl \
  gsi \
  db \
  pex \
  rdb \
  lib \
  plugins \
  unit_tests \
  buddies \
  lym \
  laybasic \
  layview \
  ant \
  img \
  edt \

equals(HAVE_RUBY, "1") {
  SUBDIRS += drc lvs
}

!equals(HAVE_QT, "0") {

  SUBDIRS += \
    klayout_main \
    lay \
    layui \
    fontgen \
    doc \
    icons \

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
pex.depends += db
rdb.depends += db
lib.depends += db

lym.depends += gsi $$LANG_DEPENDS

laybasic.depends += rdb pex
layview.depends += laybasic

ant.depends += layview
img.depends += layview
edt.depends += layview

plugins.depends += lib

equals(HAVE_PYTHON, "1") {
  pymod.depends += layview lib ant img edt lym
}

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

  layui.depends += laybasic
  layview.depends += layui
  lay.depends += ant img edt layui lym

  plugins.depends += lay

  klayout_main.depends += doc icons plugins $$MAIN_DEPENDS

} else {

  plugins.depends += layview ant img edt

}

buddies.depends += plugins pex lym $$LANG_DEPENDS
unit_tests.depends += plugins pex lym $$MAIN_DEPENDS $$LANG_DEPENDS

!equals(HAVE_QT, "0") {

  unit_tests.depends += doc icons

}

# Adds an extra target for generating the doc: "update_doc"
update_doc.commands = $$PWD/../scripts/make_drc_lvs_doc.sh
update_doc.depends = klayout_main
QMAKE_EXTRA_TARGETS += update_doc
