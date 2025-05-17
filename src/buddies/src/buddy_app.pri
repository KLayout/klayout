
include($$PWD/../../klayout.pri)

PRO_BASENAME = $$basename(_PRO_FILE_)
TARGET = $$replace(PRO_BASENAME, ".pro", "")
DESTDIR = $$OUT_PWD/../../..

include($$PWD/../../app.pri)

# On Mac OSX, these buddy tools have to be built as ordinary command line tools;
# not as bundles (*.app)
mac {
  CONFIG -= app_bundle
}

# Since the main function is entirely unspecific, we can put it into a common
# place - it's not part of the bd sources.
SOURCES = $$PWD/bd/main.cc

INCLUDEPATH += $$BD_INC $$TL_INC $$GSI_INC
DEPENDPATH += $$BD_INC $$TL_INC $$GSI_INC
LIBS += -L$$DESTDIR -lklayout_bd -lklayout_db -lklayout_pex -lklayout_tl -lklayout_gsi -lklayout_lib -lklayout_rdb -lklayout_lym

INCLUDEPATH += $$RBA_INC
DEPENDPATH += $$RBA_INC

equals(HAVE_RUBY, "1") {
  LIBS += -lklayout_rba
} else {
  LIBS += -lklayout_rbastub
}

INCLUDEPATH += $$PYA_INC
DEPENDPATH += $$PYA_INC

equals(HAVE_PYTHON, "1") {
  LIBS += -lklayout_pya
} else {
  LIBS += -lklayout_pyastub
}

DEFINES += BD_TARGET=$$TARGET

LIBS += $$RUBYLIBFILE

if(mac|linux*) {
  LIBS += -ldl
}

