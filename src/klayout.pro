
include(klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  klayout_main \
  unit_tests \
  tl \
  gsi \
  db \
  rdb \
  lym \
  laybasic \
  lay \
  ant \
  img \
  edt \
  ext \
  lib \
  plugins \
  buddies \
  drc \

LANG_DEPENDS =

equals(HAVE_RUBY, "1") {
  SUBDIRS += rba
  LANG_DEPENDS += rba
  rba.depends += gsi
} else {
  SUBDIRS += rbastub
  rbastub.depends += gsi
  LANG_DEPENDS += rbastub
}

equals(HAVE_PYTHON, "1") {
  SUBDIRS += pya
  LANG_DEPENDS += pya
  pya-unit_tests.depends += gsi
} else {
  SUBDIRS += pyastub
  pyastub.depends += gsi
  LANG_DEPENDS += pyastub
}

gsi.depends += tl
db.depends += gsi
rdb.depends += db
laybasic.depends += rdb
ant.depends += laybasic
img.depends += laybasic
edt.depends += laybasic
drc.depends += rdb lym

lym.depends += gsi $$LANG_DEPENDS
lay.depends += laybasic ant img edt lym
ext.depends += lay
lib.depends += db
buddies.depends += db

equals(HAVE_QTBINDINGS, "1") {
  SUBDIRS += gsiqt
  gsiqt.depends += gsi
  laybasic.depends += gsiqt
}

plugins.depends += lay ext lib

klayout_main.depends += plugins drc
unit_tests.depends += plugins drc

RESOURCES += \
    laybasic/layResources.qrc \
    ant/layResources.qrc
