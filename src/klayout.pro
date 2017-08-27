
include(klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  klayout_main \
  unit_tests \
  tl/tl \
  tl/unit_tests \
  gsi/gsi \
  gsi/gsi_test \
  gsi/unit_tests \
  db/db \
  db/unit_tests \
  rdb \
  lym \
  laybasic \
  lay \
  ant \
  img \
  edt \
  ext \
  lib \
  ut \
  plugins \
  buddies \
  drc \

LANG_DEPENDS =

equals(HAVE_RUBY, "1") {
  SUBDIRS += rba/rba rba/unit_tests
  rba-rba.depends += gsi/gsi
  LANG_DEPENDS += rba/rba
  rba-unit_tests.depends += ut gsi/gsi_test
} else {
  SUBDIRS += rbastub
  rbastub.depends += gsi/gsi
  LANG_DEPENDS += rbastub
}

equals(HAVE_PYTHON, "1") {
  SUBDIRS += pya/pya pya/unit_tests
  pya-pya.depends += gsi/gsi
  LANG_DEPENDS += pya/pya
  pya-unit_tests.depends += ut gsi/gsi_test
} else {
  SUBDIRS += pyastub
  pyastub.depends += gsi/gsi
  LANG_DEPENDS += pyastub
}

gsi-gsi.depends += tl/tl
gsi-gsi_test.depends += tl/tl gsi/gsi
db-db.depends += gsi/gsi
rdb.depends += db/db ut
laybasic.depends += rdb
ant.depends += laybasic
img.depends += laybasic
edt.depends += laybasic
drc.depends += rdb lym

lym.depends += gsi/gsi ut $$LANG_DEPENDS
lay.depends += laybasic ant img edt lym
ext.depends += lay
lib.depends += db/db ut
buddies.depends += db/db ut
ut.depends += db/db $$LANG_DEPENDS

equals(HAVE_QTBINDINGS, "1") {
  SUBDIRS += gsiqt
  gsiqt.depends += gsi/gsi
  laybasic.depends += gsiqt
}

# YES. It's tl-unit_tests (for tl/unit_tests)
tl-unit_tests.depends += ut
gsi-unit_tests.depends += ut gsi/gsi_test
db-unit_tests.depends += ut

plugins.depends += lay ext lib ut

klayout_main.depends += plugins drc
unit_tests.depends += plugins drc

RESOURCES += \
    laybasic/layResources.qrc \
    ant/layResources.qrc

