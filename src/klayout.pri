
equals(HAVE_QTBINDINGS, "1") {
  DEFINES += \
    HAVE_QTBINDINGS 
}

equals(HAVE_64BIT_COORD, "1") {
  DEFINES += \
    HAVE_64BIT_COORD
}

equals(HAVE_PYTHON, "1") {
  DEFINES += \
    HAVE_PYTHON 
}

equals(HAVE_RUBY, "1") {
  DEFINES += \
    HAVE_RUBY \
    HAVE_RUBY_VERSION_CODE=$$RUBYVERSIONCODE 
}

QMAKE_RPATHDIR += $$PREFIX

QMAKE_CXXFLAGS_WARN_ON += \
    -pedantic \
    -Woverloaded-virtual \
    -Wsign-promo \
    -Wsynth \
    -Wno-deprecated \
    -Wno-long-long \
    -Wno-strict-aliasing \
    -Wno-deprecated-declarations \

QT += network xml sql 

equals(HAVE_QT5, "1") {
  QT += designer printsupport
  equals(HAVE_QTBINDINGS, "1") {
    QT += multimedia multimediawidgets xmlpatterns svg gui
  }
} else {
  # questionable: use uitools instead?
  CONFIG += designer 
}

target.path = $$PREFIX
INSTALLS += target

# only support the required symbols for shared object load performance
win32 {
  QMAKE_LFLAGS += -Wl,--exclude-all-symbols
} else {
  QMAKE_CXXFLAGS += -fvisibility=hidden
}

# define our own shared object extension since QMAKE_EXTENSION_SHLIB is blank on
# Linux
win32 {
  SHLIB_EXT = dll
  SHLIB_PREFIX =
} else {
  SHLIB_EXT = so
  SHLIB_PREFIX = lib
}
export(SHLIB_EXT)
export(SHLIB_PREFIX)
