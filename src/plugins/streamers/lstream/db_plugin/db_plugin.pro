
TARGET = lstream
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)
include($$PWD/../lstream.pri)
include($$PWD/capnp/capnp.pri)

INCLUDEPATH += capnp $$VERSION_INC
LIBS += -L$$DESTDIR/.. -lxcapnp -lxkj

HEADERS += \
  lstrCompressed.h \
  lstrCompressor.h \
  lstrFormat.h \
  lstrPlugin.h \
  lstrReader.h \
  lstrWriter.h \

SOURCES += \
  gsiDeclLStream.cc \
  lstrCompressed.cc \
  lstrCompressor.cc \
  lstrFormat.cc \
  lstrPlugin.cc \
  lstrReader.cc \
  lstrWriter.cc \

