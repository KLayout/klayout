
TEMPLATE = subdirs
SUBDIRS = qtbasic 

greaterThan(QT_MAJOR_VERSION, 5) {
  SUBDIRS += qt6
  qt6.depends += qtbasic
} else {
  greaterThan(QT_MAJOR_VERSION, 4) {
    SUBDIRS += qt5
    qt5.depends += qtbasic
  } else {
    SUBDIRS += qt4
    qt4.depends += qtbasic
  }
}

