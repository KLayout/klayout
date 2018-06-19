
TARGET = gds2
DESTDIR = $$OUT_PWD/../../../../db_plugins

include($$PWD/../../../db_plugin.pri)

HEADERS = \
  contrib/dbGDS2Converter.h \
  contrib/dbGDS2TextReader.h \
  contrib/dbGDS2TextWriter.h \
  dbGDS2Format.h \
  dbGDS2.h \
  dbGDS2ReaderBase.h \
  dbGDS2Reader.h \
  dbGDS2WriterBase.h \
  dbGDS2Writer.h \

SOURCES = \
  contrib/dbGDS2Converter.cc \
  contrib/dbGDS2Text.cc \
  contrib/dbGDS2TextReader.cc \
  contrib/dbGDS2TextWriter.cc \
  dbGDS2.cc \
  dbGDS2ReaderBase.cc \
  dbGDS2Reader.cc \
  dbGDS2WriterBase.cc \
  dbGDS2Writer.cc \
  gsiDeclDbGDS2.cc \

