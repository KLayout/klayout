#!/bin/bash

#
# KLayout Layout Viewer
# Copyright (C) 2006-2026 Matthias Koefferlein
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

CURR_DIR=`pwd`
RUN_MAKE=1
IS_MAC="no"
IS_WINDOWS="no"
IS_LINUX="no"

HAVE_QTBINDINGS=1
HAVE_QT_UITOOLS=1
HAVE_QT_NETWORK=1
HAVE_QT_SQL=1
HAVE_QT_SVG=1
HAVE_QT_PRINTSUPPORT=1
HAVE_QT_MULTIMEDIA=1
HAVE_QT_DESIGNER=1
HAVE_QT_XML=1
HAVE_64BIT_COORD=0
HAVE_QT=1
HAVE_PNG=0
HAVE_CURL=0
HAVE_EXPAT=0
HAVE_GIT2=1
HAVE_LSTREAM=1

RUBYINCLUDE=""
RUBYINCLUDE2=""
RUBYLIBFILE=""
RUBYVERSIONCODE=""

PYTHONINCLUDE=""
PYTHONLIBFILE=""
PYTHONEXTSUFFIX=""

QMAKE=""
RUBY=""
PYTHON=""
BUILD=""
BIN=""
RPATH=""

MAKE_OPT=""

CONFIG="release"
BUILD_EXPERT=0

# Check if building on Mac OSX Darwin family
case `uname` in
    Linux*)
        IS_LINUX="yes"
        ;;
    MINGW*)
        IS_WINDOWS="yes"
        ;;
    CYGWIN*)
        IS_WINDOWS="yes"
        ;;
    Darwin*)
        IS_MAC="yes"
        ;;
    *)
        ;;
esac

# Check, whether build.sh is run from the top level folder
if ! [ -e src ] || ! [ -e src/klayout.pro ]; then
  echo "*** ERROR: run build.sh from the top level folder"
  exit 1
fi

while [ "$*" != "" ]; do

  a=$1
  shift

  case $a in
  -without-qt)
    HAVE_QT=0
    ;;
  -with-qtbinding)
    HAVE_QTBINDINGS=1
    ;;
  -without-qtbinding)
    HAVE_QTBINDINGS=0
    ;;
  -without-qt-uitools)
    HAVE_QT_UITOOLS=0
    ;;
  -without-qt-network)
    HAVE_QT_NETWORK=0
    ;;
  -without-qt-sql)
    HAVE_QT_SQL=0
    ;;
  -without-qt-svg)
    HAVE_QT_SVG=0
    ;;
  -without-qt-printsupport)
    HAVE_QT_PRINTSUPPORT=0
    ;;
  -without-qt-multimedia)
    HAVE_QT_MULTIMEDIA=0
    ;;
  -without-qt-designer)
    HAVE_QT_DESIGNER=0
    ;;
  -without-qt-xml)
    HAVE_QT_XML=0
    ;;
  -with-64bit-coord)
    HAVE_64BIT_COORD=1
    ;;
  -without-64bit-coord)
    HAVE_64BIT_COORD=0
    ;;
  -release)
    CONFIG="release"
    ;;
  -debug)
    CONFIG="debug"
    ;;
  -expert)
    BUILD_EXPERT=1
    ;;
  -python)
    PYTHON="$1"
    shift
    ;;
  -nopython)
    PYTHON="-"
    ;;
  -pyinc)
    PYTHONINCLUDE="$1"
    shift
    ;;
  -pylib)
    PYTHONLIBFILE="$1"
    shift
    ;;
  -pyextsuffix)
    PYTHONEXTSUFFIX="$1"
    shift
    ;;
  -qmake)
    QMAKE="$1"
    shift
    ;;
  -ruby)
    RUBY="$1"
    shift
    ;;
  -noruby)
    RUBY="-"
    ;;
  -rbvers)
    RUBYVERSIONCODE="$1"
    shift
    ;;
  -rbinc)
    RUBYINCLUDE="$1"
    shift
    ;;
  -rbinc2)
    RUBYINCLUDE2="$1"
    shift
    ;;
  -rblib)
    RUBYLIBFILE="$1"
    shift
    ;;
  -build)
    BUILD="$1"
    shift
    ;;
  -bin|-prefix)
    BIN="$1"
    shift
    ;;
  -rpath)
    RPATH="$1"
    shift
    ;;
  -dry-run)
    RUN_MAKE=0
    ;;
  -libpng)
    HAVE_PNG=1
    ;;
  -libcurl)
    HAVE_CURL=1
    ;;
  -libexpat)
    HAVE_EXPAT=1
    ;;
  -nolibgit2)
    HAVE_GIT2=0
    ;;
  -nolstream)
    HAVE_LSTREAM=0
    ;;
  -qt5)
    echo "*** WARNING: -qt5 option is ignored - Qt version is auto-detected now."
    ;;
  -qt4)
    echo "*** WARNING: -qt4 option is ignored - Qt version is auto-detected now."
    ;;
  -option)
    MAKE_OPT="$MAKE_OPT $1"
    shift
    ;;
  -h|--help|-help)
    echo "usage: build [options]"
    echo ""
    echo "options:"
    echo ""
    echo "  -debug                Run a debug build"
    echo "  -release              Run a release build [default]"
    echo ""
    echo "  -qmake <prog>         Use qmake 'prog'"
    echo ""
    echo "  -noruby               Don't build with Ruby support"
    echo "  -ruby <prog>          Use Ruby interpreter 'prog'"
    echo ""
    echo "  -nopython             Don't build with Python support"
    echo "  -python <prog>        Use Python interpreter 'prog'"
    echo ""
    echo "  -build <path>         Directory where to do the build"
    echo "  -bin|-prefix <path>   Directory where to install the binary"
    echo "  -rpath <rpath>        Specifies the RPATH to use (default: same as -bin)"
    echo "  -option <option>      'make' options (i.e. -j2)"
    echo ""
    echo "  -with-qtbinding       Create Qt bindings for ruby scripts [default]"
    echo "  -without-qtbinding    Don't create Qt bindings for ruby scripts"
    echo "  -without-qt-uitools   Don't include uitools in Qt binding"
    echo "  -with-64bit-coord     Use long (64bit) coordinates - EXPERIMENTAL FEATURE"
    echo "                          (only available for gcc>=4.4 for 64bit build)"
    echo "  -without-64bit-coord  Don't use long (64bit) coordinates [default]"
    echo "  -without-qt           Qt-less build of the core libraries (including pymod)"
    echo "                          (implies -without-qtbinding)"
    echo ""
    echo "  -dry-run              Don't build, just run qmake"
    echo ""
    echo "Special options (usually auto-selected from ruby/python/Qt installation):"
    echo ""
    echo "  -rblib <file>         Location of the .so/.dll to link for Ruby support"
    echo "  -rbinc <dir>          Location of the Ruby headers (in particular 'ruby.h')"
    echo "                          -rbinc and -rblib must be set to enable Ruby support"
    echo "  -rbinc2 <dir>         Second include path for Ruby 1.9 (containing 'ruby/config.h')"
    echo "  -rbvers <xyyzz>       Ruby version code"
    echo ""
    echo "  -pylib <file>         Location of the .so/.dll to link for Python support"
    echo "  -pyinc <dir>          Location of the Python headers (in particular 'Python.h')"
    echo ""
    echo "  -libcurl              Use libcurl instead of QtNetwork (for Qt<4.7)"
    echo "  -libexpat             Use libexpat instead of QtXml"
    echo "  -libpng               Use libpng instead of Qt for PNG generation"
    echo "  -nolibgit2            Do not include libgit2 for Git package support"
    echo "  -nolstream            Do not include the LStream plugin"
    echo ""
    echo "Environment Variables:"
    echo ""
    echo "  QMAKE_CCACHE=1        Adds CONFIG+=ccache to qmake command (only works if Qt>=5.9.2)"
    echo ""
    exit 0
    ;;
  *)
    MAKE_OPT="$MAKE_OPT $a"
    ;;
  esac

done

echo "Scanning installation .."
echo ""

# Import version info
. ./version.sh

echo "Version Info:"
echo "    Version: $KLAYOUT_VERSION"
echo "    Date: $KLAYOUT_VERSION_DATE"
echo "    Revision: $KLAYOUT_VERSION_REV"
echo ""

# if not given, try to detect the qmake binary
if [ "$QMAKE" = "" ]; then
  for qmake in "qmake5" "qmake-qt5" "qmake4" "qmake-qt4" "qmake"; do
    if [ "$QMAKE" = "" ] && [ "`$qmake -v 2>/dev/null`" != "" ]; then
      QMAKE="$qmake"
    fi
  done
fi
if [ "$QMAKE" = "" ]; then
  echo "*** ERROR: unable to find qmake tool in path"
  exit 1
fi

echo "Using qmake: $QMAKE"
echo ""

# if not given, locate ruby interpreter (prefer 1.9, then default, finally 1.8 as fallback)
if [ "$RUBY" = "" ]; then
  for ruby in "ruby2.4" "ruby2.3" "ruby2.2" "ruby2.1" "ruby2" "ruby1.9" "ruby" "ruby1.8"; do
    if [ "$RUBY" = "" ] && [ "`$ruby -e 'puts 1' 2>/dev/null`" = "1" ]; then
      RUBY="$ruby"
    fi
  done
fi
if [ "$RUBY" != "" ] && [ "$RUBY" != "-" ]; then

  echo "Using Ruby interpreter: $RUBY"

  if [ "`$RUBY -e 'puts 1' 2>/dev/null`" != "1" ]; then
    echo "*** ERROR: unable to run Ruby interpreter $RUBY"
    exit 1
  fi

  # Get ruby version
  if [ "$RUBYVERSIONCODE" = "" ]; then
    RUBYVERSIONCODE=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['MAJOR'] || 0).to_i*10000+(RbConfig::CONFIG['MINOR'] || 0).to_i*100+(RbConfig::CONFIG['TEENY'] || 0).to_i"`
  fi

  # Get ruby installation files
  if [ "$RUBYLIBFILE" = "" ]; then
    RUBYLIBFILENAME=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['LIBRUBY'] || '')"`
    RUBYLIBFILENAME_SO=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['LIBRUBY_SO'] || '')"`
    RUBYLIBFILENAME_A=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['LIBRUBY_A'] || '')"`
    if [ "$RUBYLIBFILENAME_A" = "" ] && [ "$RUBYLIBFILENAME_SO" = "" ] && [ "RUBYLIBFILENAME" = "" ]; then
      echo "*** WARNING: Could not get Ruby library name"
    else
      RUBYLIBFILEPATH=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['archlibdir'] || '')"`
      if [ "$RUBYLIBFILEPATH" = "" ] || ! [ -e "$RUBYLIBFILEPATH" ]; then
        RUBYLIBFILEPATH=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['libdir'] || '')"`
      fi
      if [ "$RUBYLIBFILEPATH" = "" ]; then
        echo "*** WARNING: Could not get Ruby library path"
      elif [ -f "$RUBYLIBFILEPATH/$RUBYLIBFILENAME" ]; then
        RUBYLIBFILE="$RUBYLIBFILEPATH/$RUBYLIBFILENAME"
        echo "    Ruby library found: $RUBYLIBFILE"
      elif [ -f "$RUBYLIBFILEPATH/$RUBYLIBFILENAME_SO" ]; then
        RUBYLIBFILE="$RUBYLIBFILEPATH/$RUBYLIBFILENAME_SO"
        echo "    Ruby library found: $RUBYLIBFILE"
      elif [ -f "$RUBYLIBFILEPATH/$RUBYLIBFILENAME_A" ]; then
        RUBYLIBFILE="$RUBYLIBFILEPATH/$RUBYLIBFILENAME_A"
        echo "    Ruby library found: $RUBYLIBFILE"
      else
        echo "*** WARNING: Could not locate Ruby library"
        echo "    Candidates are:"
        echo "      $RUBYLIBFILEPATH/$RUBYLIBFILENAME"
        echo "      $RUBYLIBFILEPATH/$RUBYLIBFILENAME_SO"
        echo "      $RUBYLIBFILEPATH/$RUBYLIBFILENAME_A"
        echo "    (none of them could be found)"
      fi
    fi
  fi

  if [ "$RUBYLIBFILE" != "" ]; then
    RUBYHDRDIR=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['rubyhdrdir'] || '')"`
    if [ "$RUBYHDRDIR" = "" ]; then
      if [ "$RUBYINCLUDE" = "" ]; then
        RUBYINCLUDE=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['archdir'] || '')"`
      fi
      echo "    Ruby headers found: $RUBYINCLUDE"
    else
      if [ "$RUBYINCLUDE" = "" ]; then
        RUBYINCLUDE="$RUBYHDRDIR"
      fi
      if [ "$RUBYINCLUDE2" = "" ]; then
        RUBYINCLUDE2=`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['rubyarchhdrdir'] || '')"`
      fi
      if [ "$RUBYINCLUDE2" = "" ]; then
        RUBYINCLUDE2="$RUBYHDRDIR"/`$RUBY -rrbconfig -e "puts (RbConfig::CONFIG['arch'] || '')"`
      fi
      echo "    Ruby headers found: $RUBYINCLUDE and $RUBYINCLUDE2"
    fi
  fi

  echo "    Ruby installation is in:"
  echo "    - $RUBYLIBFILE (lib)"
  echo "    - $RUBYINCLUDE (headers)"
  echo "    - $RUBYINCLUDE2 (arch headers)"
  echo "    Ruby version code is $RUBYVERSIONCODE"
  echo ""

fi

# if not given, locate Python interpreter
if [ "$PYTHON" = "" ]; then
  for python in "python3.5" "python3.4" "python3.3" "python3.2" "python3.1" "python3" "python2.8" "python2.7" "python2" "python"; do
    if [ "$PYTHON" = "" ] && [ "`$python -c 'print(1)' 2>/dev/null`" = "1" ]; then
      PYTHON="$python"
    fi
  done
fi
if [ "$PYTHON" != "" ] && [ "$PYTHON" != "-" ]; then

  echo "Using Python interpreter: $PYTHON"

  if [ "`$PYTHON -c 'print(1)' 2>/dev/null`" != "1" ]; then
    echo "*** ERROR: unable to run Python interpreter $PYTHON"
    exit 1
  fi

  # Get Python installation files
  if [ "$PYTHONLIBFILE" = "" ]; then
    PYTHONLIBFILENAME=`$PYTHON -c "import sysconfig; print(sysconfig.get_config_vars('LDLIBRARY')[0])" 2>/dev/null`
    if [ "$PYTHONLIBFILENAME" = "" ]; then
      PYTHONLIBFILENAME=`$PYTHON -c "import sysconfig; print(sysconfig.get_config_vars('LIBRARY')[0])" 2>/dev/null`
    fi
    if [ "$PYTHONLIBFILENAME" = "" ]; then
      PYTHONLIBFILENAME=`$PYTHON -c "import distutils.sysconfig; print(distutils.sysconfig.get_config_vars('LDLIBRARY')[0])" 2>/dev/null`
    fi
    if [ "$PYTHONLIBFILENAME" = "" ]; then
      PYTHONLIBFILENAME=`$PYTHON -c "import distutils.sysconfig; print(distutils.sysconfig.get_config_vars('LIBRARY')[0])" 2>/dev/null`
    fi
    if [ "$PYTHONLIBFILENAME" = "" ]; then
      echo "*** WARNING: Could not get Python library name"
    else
      PYTHONLIBFILEPATH=`$PYTHON -c "import sysconfig; print(sysconfig.get_config_vars('LIBDIR')[0])" 2>/dev/null`
      if [ "$PYTHONLIBFILEPATH" = "" ]; then
        PYTHONLIBFILEPATH=`$PYTHON -c "import distutils.sysconfig; print(distutils.sysconfig.get_config_vars('LIBDIR')[0])" 2>/dev/null`
      fi
      if [ "$PYTHONLIBFILEPATH" = "" ]; then
        echo "*** WARNING: Could not get Python library path"
      else
        PYTHONLIBFILE="$PYTHONLIBFILEPATH/$PYTHONLIBFILENAME"
        if [ ! -f "$PYTHONLIBFILE" ]; then
          echo "    INFO: Python library not in default path, trying to use MULTIARCH"
          PYTHONMULTIARCH=`$PYTHON -c "import sysconfig; print(sysconfig.get_config_vars('MULTIARCH')[0])" 2>/dev/null`
          PYTHONLIBFILE="$PYTHONLIBFILEPATH/$PYTHONMULTIARCH/$PYTHONLIBFILENAME"
        fi
        echo "    Python library found: $PYTHONLIBFILE"
      fi
    fi
  fi

  if [ "$PYTHONINCLUDE" = "" ]; then
    PYTHONINCLUDE=`$PYTHON -c "import distutils.sysconfig; print(distutils.sysconfig.get_config_vars('INCLUDEPY')[0])" 2>/dev/null`
    if [ "$PYTHONINCLUDE" = "" ]; then
      PYTHONINCLUDE=`$PYTHON -c "import sysconfig; print(sysconfig.get_config_vars('INCLUDEPY')[0])" 2>/dev/null`
    fi
    echo "    Python headers found: $PYTHONINCLUDE"
  fi

  if [ "$PYTHONEXTSUFFIX" = "" ]; then
    PYTHONEXTSUFFIX=$($PYTHON -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX') or sysconfig.get_config_var('SO'))" 2>/dev/null)
    if [ "$PYTHONEXTSUFFIX" = "" ]; then
      if [ "$IS_WINDOWS" = "yes" ]; then
        PYTHONEXTSUFFIX=.dll
      elif [ "$IS_MAC" = "yes" ]; then
        PYTHONEXTSUFFIX=.dylib
      else
        PYTHONEXTSUFFIX=.so
      fi
    fi
    echo "    Python extension suffix: $PYTHONEXTSUFFIX"
  fi

  echo "    Python installation is in:"
  echo "    - $PYTHONLIBFILE (lib)"
  echo "    - $PYTHONINCLUDE (includes)"
  echo ""

fi

if [ $HAVE_QT = 0 ]; then
  HAVE_QTBINDINGS=0
fi

echo "Features:"
if [ $HAVE_QT = 0 ]; then
  echo "    Qt not used at all"
fi
if [ $HAVE_QTBINDINGS != 0 ]; then
  echo "    Qt bindings enabled"
fi
if [ $HAVE_64BIT_COORD != 0 ]; then
  echo "    64 bit coordinates enabled"
fi
if [ $HAVE_EXPAT != 0 ]; then
  echo "    Uses libexpat for XML parsing"
fi
if [ $HAVE_CURL != 0 ]; then
  echo "    Uses libcurl for network access"
fi
if [ $HAVE_PNG != 0 ]; then
  echo "    Uses libpng for PNG generation"
fi
if [ $HAVE_GIT2 != 0 ]; then
  echo "    Uses libgit2 for Git access"
fi
if [ $HAVE_LSTREAM != 0 ]; then
  echo "    Includes LStream plugin"
fi
if [ "$RPATH" = "" ]; then
  RPATH="$BIN"
fi

# Check Ruby installation
if [ "$RUBYINCLUDE" != "" ]; then
  HAVE_RUBY=1
  if [ "$RUBYLIBFILE" = "" ]; then
    echo "*** ERROR: -rblib not specified or library path cannot be determined from Ruby installation"
    exit 1
  fi
  if [ "$RUBYVERSIONCODE" = "" ]; then
    echo "*** ERROR: -rbvers not specified or version cannot be determined from Ruby installation"
    exit 1
  fi
  if [ ! -f "$RUBYINCLUDE/ruby.h" ]; then
    echo "*** ERROR: wrong -rbinc path: $RUBYINCLUDE/ruby.h does not exist or is not a file"
    exit 1
  fi
  if [ "$RUBYINCLUDE2" != "" ] && [ ! -f "$RUBYINCLUDE2/ruby/config.h" ]; then
    echo "*** ERROR: wrong -rbinc2 path: $RUBYINCLUDE2/ruby/config.h does not exist or is not a file"
    exit 1
  fi
  if [ ! -f "$RUBYLIBFILE" ]; then
    echo "*** ERROR: wrong -rblib path: $RUBYLIBFILE does not exist or not a file"
    exit 1
  fi
else
  HAVE_RUBY=0
fi

# Check Python installation
if [ "$PYTHONINCLUDE" != "" ]; then
  HAVE_PYTHON=1
  if [ "$PYTHONLIBFILE" = "" ]; then
    echo "*** ERROR: -pylib not specified or library path cannot be determined from Python installation"
    exit 1
  fi
  if [ ! -f "$PYTHONINCLUDE/Python.h" ]; then
    echo "*** ERROR: wrong -pyinc path: $PYTHONINCLUDE/Python does not exist or is not a file"
    exit 1
  fi
  if [ ! -f "$PYTHONLIBFILE" ]; then
    echo "*** ERROR: wrong -pylib path: $PYTHONLIBFILE does not exist or not a file"
    exit 1
  fi
else
  HAVE_PYTHON=0
fi

if [ "$BUILD" = "" ]; then
  BUILD=$CURR_DIR/build-$CONFIG
fi

if [ "$BIN" = "" ]; then
  BIN=$CURR_DIR/bin-$CONFIG
fi

if [ "$QMAKE_CCACHE" = 1 ]; then
  echo "    Compilation caching is activated."
else
  echo "    Compilation caching is deactivated!"
fi
echo "    Installation target: $BIN"
echo "    Build directory: $BUILD"
echo ""
echo "    Build flags:"
echo "      HAVE_RUBY=$HAVE_RUBY"
echo "      HAVE_PYTHON=$HAVE_PYTHON"
echo "      HAVE_QTBINDINGS=$HAVE_QTBINDINGS"
echo "      HAVE_QT=$HAVE_QT"
echo "      HAVE_QT_UITOOLS=$HAVE_QT_UITOOLS"
echo "      HAVE_QT_NETWORK=$HAVE_QT_NETWORK"
echo "      HAVE_QT_SQL=$HAVE_QT_SQL"
echo "      HAVE_QT_SVG=$HAVE_QT_SVG"
echo "      HAVE_QT_PRINTSUPPORT=$HAVE_QT_PRINTSUPPORT"
echo "      HAVE_QT_MULTIMEDIA=$HAVE_QT_MULTIMEDIA"
echo "      HAVE_QT_DESIGNER=$HAVE_QT_DESIGNER"
echo "      HAVE_QT_XML=$HAVE_QT_XML"
echo "      HAVE_64BIT_COORD=$HAVE_64BIT_COORD"
echo "      HAVE_CURL=$HAVE_CURL"
echo "      HAVE_PNG=$HAVE_PNG"
echo "      HAVE_EXPAT=$HAVE_EXPAT"
echo "      HAVE_GIT2=$HAVE_GIT2"
echo "      HAVE_LSTREAM=$HAVE_LSTREAM"
echo "      RPATH=$RPATH"

mkdir -p $BUILD

# source the version script
. $(dirname $(which $0))/version.sh

# qmake needs absolute paths, so we get them now:
#   OSX does not have `readlink -f` command. Use equivalent Perl script.
if [ "$IS_MAC" = "no" ]; then
  BUILD=`readlink -f $BUILD`
  BIN=`readlink -f $BIN`
else
  BUILD=`perl -MCwd -le 'print Cwd::abs_path(shift)' $BUILD`
  BIN=`perl -MCwd -le 'print Cwd::abs_path(shift)' $BIN`
fi

if [ "$IS_MAC" = "no" ]; then
  if ( gmake -v >/dev/null 2>/dev/null ); then
    MAKE_PRG=gmake
  else
    MAKE_PRG=make
  fi
else
  MAKE_PRG=make
fi

PLUGINS=""
cd $CURR_DIR/src/plugins
for plugin in `echo *`; do
  if [ -e $plugin/$plugin.pro ]; then
    PLUGINS="$PLUGINS$plugin "
  fi
done
cd $CURR_DIR

echo "    Building plugins: $PLUGINS"
echo ""

echo "Running $QMAKE .."
cd $BUILD

$QMAKE -v

# Force a minimum rebuild because of version info
touch $CURR_DIR/src/version/version.h

qmake_options=(
  -recursive
  CONFIG+="$CONFIG"
  RUBYLIBFILE="$RUBYLIBFILE"
  RUBYVERSIONCODE="$RUBYVERSIONCODE"
  HAVE_RUBY="$HAVE_RUBY"
  PYTHON="$PYTHON"
  PYTHONLIBFILE="$PYTHONLIBFILE"
  PYTHONINCLUDE="$PYTHONINCLUDE"
  PYTHONEXTSUFFIX="$PYTHONEXTSUFFIX"
  HAVE_PYTHON="$HAVE_PYTHON"
  HAVE_QTBINDINGS="$HAVE_QTBINDINGS"
  HAVE_QT_UITOOLS="$HAVE_QT_UITOOLS"
  HAVE_QT_NETWORK="$HAVE_QT_NETWORK"
  HAVE_QT_SQL="$HAVE_QT_SQL"
  HAVE_QT_SVG="$HAVE_QT_SVG"
  HAVE_QT_PRINTSUPPORT="$HAVE_QT_PRINTSUPPORT"
  HAVE_QT_MULTIMEDIA="$HAVE_QT_MULTIMEDIA"
  HAVE_QT_DESIGNER="$HAVE_QT_DESIGNER"
  HAVE_QT_XML="$HAVE_QT_XML"
  HAVE_64BIT_COORD="$HAVE_64BIT_COORD"
  HAVE_QT="$HAVE_QT"
  HAVE_CURL="$HAVE_CURL"
  HAVE_EXPAT="$HAVE_EXPAT"
  HAVE_PNG="$HAVE_PNG"
  HAVE_GIT2="$HAVE_GIT2"
  HAVE_LSTREAM="$HAVE_LSTREAM"
  PREFIX="$BIN"
  RPATH="$RPATH"
  KLAYOUT_VERSION="$KLAYOUT_VERSION"
  KLAYOUT_VERSION_DATE="$KLAYOUT_VERSION_DATE"
  KLAYOUT_VERSION_REV="$KLAYOUT_VERSION_REV"
)

# NOTE: qmake does not like include paths which clash with paths built into the compiler
# hence we don't add RUBYINCLUDE or RUBYINCLUDE2 in this case (found on CentOS 8 where Ruby
# headers are installed in /usr/include)
if [ "$RUBYINCLUDE" != "/usr/include" ] && [  "$RUBYINCLUDE" != "/usr/local/include" ]; then
  qmake_options+=( RUBYINCLUDE="$RUBYINCLUDE" )
fi
if [ "$RUBYINCLUDE2" != "/usr/include" ] && [  "$RUBYINCLUDE2" != "/usr/local/include" ]; then
  qmake_options+=( RUBYINCLUDE2="$RUBYINCLUDE2" )
fi

# This should speed up build time considerably
# https://ortogonal.github.io/ccache-and-qmake-qtcreator/
if [ "$QMAKE_CCACHE" = 1 ]; then
  qmake_options+=(
    CONFIG+="ccache"
  )
fi

if [ $BUILD_EXPERT = 1 ]; then
  qmake_options+=(
    QMAKE_AR="$AR cqs"
    QMAKE_LINK_C="$CC"
    QMAKE_LINK_C_SHLIB="$CC"
    QMAKE_LINK="$CXX"
    QMAKE_LINK_SHLIB="$CXX"
    QMAKE_OBJCOPY="$OBJCOPY"
    QMAKE_RANLIB=
    QMAKE_STRIP=
    QMAKE_CC="$CC"
    QMAKE_CXX="$CXX"
    QMAKE_CFLAGS="$CFLAGS"
    QMAKE_CFLAGS_RELEASE=
    QMAKE_CFLAGS_DEBUG=
    QMAKE_CXXFLAGS="$CXXFLAGS"
    QMAKE_CXXFLAGS_RELEASE=
    QMAKE_CXXFLAGS_DEBUG=
    QMAKE_LIBS="$LIBS"
    QMAKE_LFLAGS="$LDFLAGS"
    QMAKE_LFLAGS_RELEASE=
    QMAKE_LFLAGS_DEBUG=
  )
fi

echo $QMAKE "$CURR_DIR/src/klayout.pro" "${qmake_options[@]}"
$QMAKE "$CURR_DIR/src/klayout.pro" "${qmake_options[@]}"

cd $CURR_DIR
echo ""

if [ $RUN_MAKE = 0 ]; then
  exit 0
fi

# -- Running build  --

echo "Running build ($MAKE_PRG $MAKE_OPT all) .."
cd $BUILD
$MAKE_PRG $MAKE_OPT || exit 1
cd $CURR_DIR
echo ""

# -- Installing binaries  --

echo "Installing binaries .."
cd $BUILD
$MAKE_PRG install
cd $CURR_DIR
echo ""
echo "Build successfully done."
echo "Artefacts were installed to $BIN"

exit 0

