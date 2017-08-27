
include(klayout.pri)

TEMPLATE = subdirs

SUBDIRS = \
  klayout_main \
  unit_tests \
  tl/tl \
  tl/unit_tests \
  gsi/gsi \
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

equals(HAVE_RUBY, "1") {
  SUBDIRS += rba
  rba.depends += gsi/gsi ut
} else {
  SUBDIRS += rbastub
  rbastub.depends += gsi/gsi
}

equals(HAVE_PYTHON, "1") {
  SUBDIRS += pya
  pya.depends += gsi/gsi ut
} else {
  SUBDIRS += pyastub
  pyastub.depends += gsi/gsi
}

gsi-gsi.depends += tl/tl
db-db.depends += gsi/gsi
rdb.depends += db/db
laybasic.depends += db/db rdb
ant.depends += laybasic
img.depends += laybasic
edt.depends += laybasic
drc.depends += rdb lym

lym.depends += gsi/gsi ut
equals(HAVE_RUBY, "1") {
  lym.depends += rba
} else {
  lym.depends += rbastub
}
equals(HAVE_PYTHON, "1") {
  lym.depends += pya
} else {
  lym.depends += pyastub
}

lay.depends += laybasic ant img edt lym
ext.depends += lay
lib.depends += db/db ut
buddies.depends += db/db ut

equals(HAVE_QTBINDINGS, "1") {
  SUBDIRS += gsiqt
  gsiqt.depends += gsi/gsi
  laybasic.depends += gsiqt
  lay.depends += gsiqt
}

ut.depends += db/db

# YES. It's tl-unit_tests (for tl/unit_tests)
tl-unit_tests.depends += ut
gsi-unit_tests.depends += ut
db-unit_tests.depends += ut

plugins.depends += lay ext lib ut

klayout_main.depends += lay ext lib plugins
unit_tests.depends += ut lay ext lib

RESOURCES += \
    laybasic/layResources.qrc \
    ant/layResources.qrc

