#!/bin/sh

# 
# KLayout Layout Viewer
# Copyright (C) 2006-2017 Matthias Koefferlein
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

HAVE_QTBINDINGS=1
HAVE_64BIT_COORD=0
HAVE_QT5=""

RUBYINCLUDE=""
RUBYINCLUDE2=""
RUBYLIBFILE=""
RUBYVERSIONCODE=""

PYTHONINCLUDE=""
PYTHONLIBFILE=""

QMAKE="qmake"
RUBY=""
PYTHON=""
BUILD=""
BIN=""

MAKE_OPT=""

CONFIG="release"

# Check, whether build.sh is run from the top level folder
if ! [ -e src ] || ! [ -e src/klayout.pro ]; then
  echo "*** ERROR: run build.sh from the top level folder"
  exit 1
fi

while [ "$*" != "" ]; do

  a=$1
  shift

  case $a in
  -with-qtbinding)
    HAVE_QTBINDINGS=1
    ;;
  -without-qtbinding)
    HAVE_QTBINDINGS=0
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
  -qt4)
    HAVE_QT5=0
    ;;
  -qt5)
    HAVE_QT5=1
    ;;
  -dry-run)
    RUN_MAKE=0
    ;;
  -option)
    MAKE_OPT="$MAKE_OPT $1"
    shift
    ;;
  -h|--help|-help)
    echo "usage: build [options]"
    echo ""
    echo "options:"
    echo "  -debug                Run a debug build"
    echo "  -release              Run a release build [default]"
    echo "  -qmake <prog>         Use qmake 'prog'"
    echo "  -noruby               Don't build with Ruby support"
    echo "  -ruby <prog>          Use Ruby interpreter 'prog'"
    echo "  -nopython             Don't build with Python support"
    echo "  -python <prog>        Use Python interpreter 'prog'"
    echo "  -build <path>         Directory where to do the build"
    echo "  -bin|-prefix <path>   Directory where to install the binary"
    echo "  -option <option>      'make' options (i.e. -j2)"
    echo ""
    echo "  -with-qtbinding       Create Qt bindings for ruby scripts [default]"
    echo "  -without-qtbinding    Don't create Qt bindings for ruby scripts"
    echo "  -with-64bit-coord     Use long (64bit) coordinates - EXPERIMENTAL FEATURE"
    echo "                        (only available for gcc>=4.4 for 64bit build)"
    echo "  -without-64bit-coord  Don't use long (64bit) coordinates [default]"
    echo ""
    echo "  -qt4|-qt5             Use Qt4 or Qt5 API [default: auto detect]"
    echo ""
    echo "  -dry-run              Don't build, just run qmake"
    echo ""
    echo "Special options (normally determined from ruby installation):"
    echo "  -rblib <file>         Location of the .so/.dll to link for Ruby support"
    echo "  -rbinc <dir>          Location of the Ruby headers (in particular 'ruby.h')"
    echo "                        -rbinc and -rblib must be set to enable Ruby support"
    echo "  -rbinc2 <dir>         Second include path for Ruby 1.9 (containing 'ruby/config.h')"
    echo "  -rbvers <xyyzz>       Ruby version code"
    echo "  -pylib <file>         Location of the .so/.dll to link for Python support"
    echo "  -pyinc <dir>          Location of the Python headers (in particular 'Python.h')"
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

# if not given, try to detect the qt major version to use
if [ "$HAVE_QT5" = "" ]; then
  qt_major=`$QMAKE -v | grep 'Using Qt version' | sed 's/.*version  *\([0-9][0-9]*\).*/\1/'`
  if [ "$qt_major" = "4" ]; then
    HAVE_QT5=0
    echo "Using Qt 4 API"
    echo ""
  elif [ "$qt_major" = "5" ]; then
    HAVE_QT5=1
    echo "Using Qt 5 API"
    echo ""
  else
    echo "*** ERROR: could not determine Qt version from '$QMAKE -v'"
    exit 1
  fi
fi

# if not given, locate ruby interpreter (prefer 1.9, then default, finally 1.8 as fallback)
if [ "$RUBY" != "-" ]; then
  for ruby in "ruby1.9" "ruby" "ruby1.8"; do
    if [ "$RUBY" = "" ] && [ "`$ruby -v 2>/dev/null`" != "" ]; then 
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
if [ "$PYTHON" != "-" ]; then
  for python in "python"; do
    if [ "$PYTHON" = "" ] && [ "`$python -V 2>&1`" != "" ]; then 
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

  echo "    Python installation is in:"
  echo "    - $PYTHONLIBFILE (lib)"
  echo "    - $PYTHONINCLUDE (includes)"
  echo ""

fi

if [ $HAVE_QTBINDINGS != 0 ]; then
  echo "    Qt bindings enabled"
fi
if [ $HAVE_64BIT_COORD != 0 ]; then
  echo "    64 bit coordinates enabled"
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

mkdir -p $BUILD

# source the version script
. $(dirname $(which $0))/version.sh

# qmake needs absolute paths, so we get them now:
BUILD=`readlink -f $BUILD`
BIN=`readlink -f $BIN`

if ( gmake -v >/dev/null 2>/dev/null ); then
  MAKE_PRG=gmake
else
  MAKE_PRG=make
fi

PLUGINS=""
cd $CURR_DIR/src/plugins
for plugin in `echo *`; do
  if [ -e $plugin/$plugin.pro ]; then
    PLUGINS="$PLUGINS $plugin"
  fi
done
cd $CURR_DIR

echo "    Building plugins: $PLUGINS"
echo ""

echo "Running $QMAKE .."
cd $BUILD

# chose the right qmake
if [ $HAVE_QT5 = 0 ]; then
  export QT_SELECT=4
else
  export QT_SELECT=5
fi

$QMAKE -v

qmake_cmd="$QMAKE $CURR_DIR/src/klayout.pro -recursive \
  CONFIG+=$CONFIG \
  RUBYLIBFILE=$RUBYLIBFILE \
  RUBYINCLUDE=$RUBYINCLUDE \
  RUBYINCLUDE2=$RUBYINCLUDE2 \
  RUBYVERSIONCODE=$RUBYVERSIONCODE \
  HAVE_RUBY=$HAVE_RUBY \
  PYTHONLIBFILE=$PYTHONLIBFILE \
  PYTHONINCLUDE=$PYTHONINCLUDE \
  HAVE_PYTHON=$HAVE_PYTHON \
  HAVE_QTBINDINGS=$HAVE_QTBINDINGS \
  HAVE_64BIT_COORD=$HAVE_64BIT_COORD \
  HAVE_QT5=$HAVE_QT5 \
  PREFIX=$BIN \
"

echo $qmake_cmd
$qmake_cmd

cd $CURR_DIR
echo ""

if [ $RUN_MAKE = 0 ]; then
  exit 0
fi

# -- Running build  --

echo "Running build ($MAKE_PRG $MAKE_OPT all) .."
cd $BUILD
$MAKE_PRG $MAKE_OPT || exit
cd $CURR_DIR
echo ""

# -- Installing binaries  --

echo "Installing binaries .."
cd $BUILD
$MAKE_PRG install
cd $CURR_DIR
echo ""
echo "Build done."

exit 0

