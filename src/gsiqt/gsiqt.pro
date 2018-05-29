
TEMPLATE = subdirs
SUBDIRS = qtbasic 

equals(HAVE_QT5, "1") {
  SUBDIRS += qt5
  qt5.depends += qtbasic
} else {
  SUBDIRS += qt4
  qt4.depends += qtbasic
}

