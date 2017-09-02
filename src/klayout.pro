
include(klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  klayout_main \
  unit_tests \
  tl \
  gsi/gsi \
  gsi/gsi_test \
  gsi/unit_tests \
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
  rba.depends += gsi/gsi gsi/gsi_test
} else {
  SUBDIRS += rbastub
  rbastub.depends += gsi/gsi
  LANG_DEPENDS += rbastub
}

equals(HAVE_PYTHON, "1") {
  SUBDIRS += pya
  LANG_DEPENDS += pya
  pya-unit_tests.depends += gsi/gsi gsi/gsi_test
} else {
  SUBDIRS += pyastub
  pyastub.depends += gsi/gsi
  LANG_DEPENDS += pyastub
}

gsi-gsi.depends += tl
gsi-gsi_test.depends += tl gsi/gsi
db.depends += gsi/gsi
rdb.depends += db
laybasic.depends += rdb
ant.depends += laybasic
img.depends += laybasic
edt.depends += laybasic
drc.depends += rdb lym

lym.depends += gsi/gsi $$LANG_DEPENDS
lay.depends += laybasic ant img edt lym
ext.depends += lay
lib.depends += db
buddies.depends += db

equals(HAVE_QTBINDINGS, "1") {
  SUBDIRS += gsiqt
  gsiqt.depends += gsi/gsi
  laybasic.depends += gsiqt
}

# YES. It's gsi-unit_tests (for gsi/unit_tests)
gsi-unit_tests.depends += gsi/gsi_test

plugins.depends += lay ext lib

klayout_main.depends += plugins drc
unit_tests.depends += plugins drc

RESOURCES += \
    laybasic/layResources.qrc \
    ant/layResources.qrc
