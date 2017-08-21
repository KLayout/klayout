
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

# only support the required symbols for shared object load performance
win32 {
  QMAKE_LFLAGS += -Wl,--exclude-all-symbols
} else {
  QMAKE_CXXFLAGS += -fvisibility=hidden
}

DEFINES += \
  KLAYOUT_VERSION=$$(KLAYOUT_VERSION) \
  KLAYOUT_VERSION_REV=$$(KLAYOUT_VERSION_REV) \
  KLAYOUT_VERSION_DATE=$$(KLAYOUT_VERSION_DATE) \
