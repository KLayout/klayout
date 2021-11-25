
TEMPLATE = subdirs

contains(QT_CONFIG, opengl) {

  greaterThan(QT_MAJOR_VERSION, 4) {
    SUBDIRS = lay_plugin unit_tests
  }

  greaterThan(QT_MAJOR_VERSION, 5) {
    QT += openglwidgets
  }

  unit_tests.depends += lay_plugin

}
