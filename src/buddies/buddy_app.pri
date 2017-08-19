
include($$PWD/../klayout.pri)

TEMPLATE = app

PRO_BASENAME = $$basename(_PRO_FILE_)
TARGET = $$replace(PRO_BASENAME, ".pro", "")
DESTDIR = $$OUT_PWD/../..

# Since the main function is entirely unspecific, we can put it into a common
# place - it's not part of the bd sources.
SOURCES = $$PWD/bd/main.cc

INCLUDEPATH += ../bd
DEPENDPATH += ../bd 
LIBS += -L$$DESTDIR -lklayout_bd 

DEFINES += BD_TARGET=$$TARGET
