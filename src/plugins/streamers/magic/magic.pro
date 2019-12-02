
TEMPLATE = subdirs

SUBDIRS = db_plugin unit_tests
unit_tests.depends += db_plugin

!equals(HAVE_QT, "0") {
  SUBDIRS += lay_plugin
  lay_plugin.depends += db_plugin
}
