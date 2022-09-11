
DESTDIR = $$OUT_PWD/..
TARGET = klayout_doc

include($$PWD/../lib.pri)

DEFINES += MAKE_DOC_LIBRARY

HEADERS = \

FORMS = \

SOURCES = \
    docForceLink.cc \

RESOURCES = \
    docResources.qrc \
    docDRCLVSResources.qrc \

