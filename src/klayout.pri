
TL_INC = $$PWD/tl/tl
DB_INC = $$PWD/db/db
PEX_INC = $$PWD/pex/pex
DRC_INC = $$PWD/drc/drc
LVS_INC = $$PWD/lvs/lvs
EDT_INC = $$PWD/edt/edt
EXT_INC = $$PWD/ext/ext
GSI_INC = $$PWD/gsi/gsi
ANT_INC = $$PWD/ant/ant
RDB_INC = $$PWD/rdb/rdb
IMG_INC = $$PWD/img/img
LYM_INC = $$PWD/lym/lym
LIB_INC = $$PWD/lib/lib
LAYBASIC_INC = $$PWD/laybasic/laybasic
LAYVIEW_INC = $$PWD/layview/layview
LAYUI_INC = $$PWD/layui/layui
LAY_INC = $$PWD/lay/lay
DOC_INC = $$PWD/doc
ICONS_INC = $$PWD/icons

QTBASIC_INC = $$PWD/gsiqt/qtbasic

BD_INC = $$PWD/buddies/src/bd

VERSION_INC = $$PWD/version

equals(HAVE_RUBY, "1") {
  RBA_INC = $$PWD/rba/rba
  RBA_LIB = klayout_rba
} else {
  RBA_INC = $$PWD/rbastub
  RBA_LIB = klayout_rbastub
}

equals(HAVE_PYTHON, "1") {
  PYA_INC = $$PWD/pya/pya
  PYA_LIB = klayout_pya
} else {
  PYA_INC = $$PWD/pyastub
  PYA_LIB = klayout_pyastub
}

equals(HAVE_QTBINDINGS, "1") {
  DEFINES += HAVE_QTBINDINGS
}

!equals(HAVE_QT_UITOOLS, "0") {
  # auto-select uitools, not all distributions have it
  contains(QT_MODULES, "uitools") {
    HAVE_QT_UITOOLS = 1
  } else {
    HAVE_QT_UITOOLS = 0
  }
}

equals(HAVE_64BIT_COORD, "1") {
  DEFINES += HAVE_64BIT_COORD
}

equals(HAVE_PYTHON, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/python/python.pri)
  }
  DEFINES += HAVE_PYTHON
}

equals(HAVE_CURL, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/curl/curl.pri)
  } else {
    LIBS += -lcurl
    win32 {
      LIBS += -lwsock32    # required because we do "select"
    }
  }
  DEFINES += HAVE_CURL
}

equals(HAVE_PNG, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/png/png.pri)
  } else {
    LIBS += -lpng
  }
  DEFINES += HAVE_PNG
}

equals(HAVE_EXPAT, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/expat/expat.pri)
  } else {
    LIBS += -lexpat
  }
  DEFINES += HAVE_EXPAT
}

equals(HAVE_PTHREADS, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/ptw/ptw.pri)
  } else {
    LIBS += -lpthread
  }
  DEFINES += HAVE_PTHREADS
}

equals(HAVE_GIT2, "1") {
  !mac {
    !isEmpty(BITS_PATH) {
      include($$BITS_PATH/git2/git2.pri)
    } else {
      LIBS += -lgit2
    }
  } else {
    QMAKE_INCDIR += $$(MAC_LIBGIT2_INC)
    QMAKE_LFLAGS += -L$$(MAC_LIBGIT2_LIB)
    LIBS += -lgit2
  }
  DEFINES += HAVE_GIT2
}

# Use the Address Sanitizer for the debug build on Mac
mac {
  USE_ASAN_MAC = $$system(echo $$(MAC_USE_ASAN))
  equals(USE_ASAN_MAC, "1") {
    QMAKE_CXXFLAGS += -fsanitize=address
    QMAKE_LFLAGS += -fsanitize=address
  }
}

equals(HAVE_RUBY, "1") {
  !isEmpty(BITS_PATH) {
    include($$BITS_PATH/ruby/ruby.pri)
  }
  DEFINES += \
    HAVE_RUBY \
    HAVE_RUBY_VERSION_CODE=$$RUBYVERSIONCODE 
}

!isEmpty(RPATH) {
  QMAKE_RPATHDIR += $$RPATH
}

!isEmpty(BITS_PATH) {
  include($$BITS_PATH/zlib/zlib.pri)
} else {
  !msvc {
    LIBS += -lz
  }
}

equals(HAVE_CRONOLOGY, "1") {
  DEFINES += HAVE_CRONOLOGY
  LIBS += $$CRONOLOGY_LIB
  INCLUDEPATH += $$CRONOLOGY_INCLUDE
}

!lessThan(QT_MAJOR_VERSION, 6) {
  # internal resource readers to not support zstd so far
  QMAKE_RESOURCE_FLAGS += --compress-algo zlib
}

msvc {

  QMAKE_CXXFLAGS += \
      /bigobj \
      /wd4251                     # Disable "DLL interface required"

  lessThan(QT_MAJOR_VERSION, 6) {
    # MSVC 2017 needs this option:
    QMAKE_CXXFLAGS += \
        /source-charset:utf-8 \     # Use UTF-8 for source files
  }

  QMAKE_LFLAGS += \
      /OPT:NOREF \                # preserves functions we want to keep for implementing the "force link" mechanism

  QMAKE_CXXFLAGS_WARN_ON += \

  # for stack trace support:
  # lpsapi for GetModuleFileName and others
  # dbghelp for SymFromAddr and other
  LIBS += -lpsapi -ldbghelp

} else {

  CONFIG(gcov) {
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage
    QMAKE_LFLAGS += --coverage
  }

  QMAKE_CXXFLAGS_WARN_ON += \
      -pedantic \
      -Woverloaded-virtual \
      -Wsynth \
      -Wno-deprecated \
      -Wno-long-long \
      -Wno-strict-aliasing \
      -Wno-deprecated-declarations \

  # too noisy on Qt:
  # QMAKE_CXXFLAGS_WARN_ON += \
  #     -Wsign-promo \
  #     -Wno-reserved-user-defined-literal \
  #

  lessThan(QT_MAJOR_VERSION, 6) {
    # because we use unordered_map/unordered_set:
    QMAKE_CXXFLAGS += -std=c++11
  } else {
    # because we use unordered_map/unordered_set:
    QMAKE_CXXFLAGS += -std=c++17
  }

  win32 {

    QMAKE_LFLAGS += -Wl,--exclude-all-symbols

    # for stack trace support:
    # lpsapi for GetModuleFileName and others
    # dbghelp for SymFromAddr and other
    LIBS += -lpsapi -ldbghelp

    QMAKE_CXXFLAGS += -Wa,-mbig-obj

  } else {

    QMAKE_CXXFLAGS += -fvisibility=hidden

  }

  *bsd* {
    # stack trace support
    LIBS += -lexecinfo
  }

}

equals(HAVE_QT, "0") {

  QT =

  # fake qrc made with python
  !equals(HAVE_PYTHON, "0") {
    new_qrc.output  = qrc_${QMAKE_FILE_BASE}.cc
    new_qrc.commands = $$PYTHON $$PWD/../scripts/pyqrc.py ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}
    new_qrc.depend_command = $$PYTHON $$PWD/../scripts/pyqrc.py ${QMAKE_FILE_NAME}
    new_qrc.input = RESOURCES
    new_qrc.variable_out = SOURCES
    new_qrc.CONFIG += dep_lines
    QMAKE_EXTRA_COMPILERS += new_qrc
  }

} else {

  DEFINES += HAVE_QT
  QT += core xml network

  greaterThan(QT_MAJOR_VERSION, 5) {
    QT += core5compat
  }

  equals(HAVE_QTBINDINGS, "1") {
    # sql isn't needed by the base application
    !equals(HAVE_QT_SQL, "0") {
      QT += sql
    }
  }

  greaterThan(QT_MAJOR_VERSION, 4) {

    QT += widgets gui printsupport

    equals(HAVE_QTBINDINGS, "1") {
      !equals(HAVE_QT_DESIGNER, "0") {
        # designer isn't needed by the base application
        QT += designer
      }
      !equals(HAVE_QT_MULTIMEDIA, "0") {
        # multimedia isn't needed by the base application
        QT += multimedia multimediawidgets
      }
      !equals(HAVE_QT_SVG, "0") {
        # svg isn't needed by the base application
        QT += svg
      }
      !equals(HAVE_QT_UITOOLS, "0") {
        # uitools isn't needed by the base application
        QT += uitools
      }
      !equals(HAVE_QT_XML, "0") {
        lessThan(QT_MAJOR_VERSION, 6) {
          QT += xmlpatterns
        }
      }

    }

  } else {

    equals(HAVE_QTBINDINGS, "1") {
      !equals(HAVE_QT_DESIGNER, "0") {
        # designer isn't needed by the base application
        CONFIG += designer
      }
      !equals(HAVE_QT_UITOOLS, "0") {
        # uitools isn't needed by the base application
        CONFIG += uitools
      }
    }

  }

  !equals(HAVE_QT_UITOOLS, "0") {
    DEFINES += HAVE_QT_UITOOLS
  }
  !equals(HAVE_QT_NETWORK, "0") {
    DEFINES += HAVE_QT_NETWORK
  }
  !equals(HAVE_QT_SQL, "0") {
    DEFINES += HAVE_QT_SQL
  }
  !equals(HAVE_QT_SVG, "0") {
    DEFINES += HAVE_QT_SVG
  }
  !equals(HAVE_QT_PRINTSUPPORT, "0") {
    DEFINES += HAVE_QT_PRINTSUPPORT
  }
  !equals(HAVE_QT_MULTIMEDIA, "0") {
    DEFINES += HAVE_QT_MULTIMEDIA
  }
  !equals(HAVE_QT_DESIGNER, "0") {
    DEFINES += HAVE_QT_DESIGNER
  }
  !equals(HAVE_QT_XML, "0") {
    DEFINES += HAVE_QT_XML
  }
}

VERSION_STRING = $$KLAYOUT_VERSION
VERSION_STRING_WITH_BLANKS = $$replace(VERSION_STRING, "\\.", " ")
eval(VERSION_ARRAY = $$VERSION_STRING_WITH_BLANKS)

KLAYOUT_MAJOR_VERSION = $$member(VERSION_ARRAY, 0)

KLAYOUT_MINOR_VERSION = $$member(VERSION_ARRAY, 1)

KLAYOUT_TINY_VERSION = $$member(VERSION_ARRAY, 2)
isEmpty(KLAYOUT_TINY_VERSION) {
  KLAYOUT_TINY_VERSION = 0
}

DEFINES += \
  KLAYOUT_VERSION=$$KLAYOUT_VERSION \
  KLAYOUT_VERSION_REV=$$KLAYOUT_VERSION_REV \
  KLAYOUT_VERSION_DATE=$$KLAYOUT_VERSION_DATE \
  KLAYOUT_MAJOR_VERSION=$$KLAYOUT_MAJOR_VERSION \
  KLAYOUT_MINOR_VERSION=$$KLAYOUT_MINOR_VERSION \
  KLAYOUT_TINY_VERSION=$$KLAYOUT_TINY_VERSION \

VERSION = $$KLAYOUT_VERSION

CONFIG(debug, debug|release) {
  DEFINES += HAVE_DEBUG
}
