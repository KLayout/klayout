
TEMPLATE = subdirs

!equals(HAVE_LSTREAM, "0") {

  SUBDIRS = runtime db_plugin unit_tests
  db_plugin.depends += runtime
  unit_tests.depends += db_plugin

  !equals(HAVE_QT, "0") {
    SUBDIRS += lay_plugin
    lay_plugin.depends += db_plugin
  }

}

