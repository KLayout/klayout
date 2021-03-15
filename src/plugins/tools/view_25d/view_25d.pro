
TEMPLATE = subdirs

contains(QT_CONFIG, opengl) {

  equals(HAVE_QT5, "1") {
    SUBDIRS = lay_plugin unit_tests
  }

  unit_tests.depends += lay_plugin

}
