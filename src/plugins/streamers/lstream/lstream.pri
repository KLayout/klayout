
!msvc {

  # capnp needs C++ 14 in version 1.0.1
  # Qt6 comes with C++ 17 requirement.
  equals(HAVE_QT, "0") || lessThan(QT_MAJOR_VERSION, 6) {
    # (1) and (2) are required by (macOS Sonoma) x (Qt5 MacPorts)
    CONFIG -= c++11 # (1)
    CONFIG += c++14 # (2)
    QMAKE_CXXFLAGS += -std=c++14
  }

  # capnp runtimes have some unused arguments
  QMAKE_CXXFLAGS += \
      -Wno-unused-parameter
}

 INCLUDEPATH += $$PWD/runtime/capnp $$PWD/runtime/kj
