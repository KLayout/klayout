
TEMPLATE = subdirs

contains(QT_CONFIG, opengl) {

  greaterThan(QT_MAJOR_VERSION, 4) {
    SUBDIRS = lay_plugin unit_tests
  }

  unit_tests.depends += lay_plugin

}
