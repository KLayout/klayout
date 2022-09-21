
DESTDIR = $$OUT_PWD/..
TARGET = klayout_icons

include($$PWD/../lib.pri)

DEFINES += MAKE_ICONS_LIBRARY

HEADERS = \

FORMS = \

SOURCES = \
    iconsForceLink.cc \

RESOURCES = \
    icons.qrc \
