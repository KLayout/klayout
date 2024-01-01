#!/bin/bash -e

# 
# A script to produce the Qt declaration files
#
# This script utilizes the extraction framework from mkqtdecl_common. The extraction
# framework is based on a C++ parser and a configuration file that specifies details
# about the translation.
#
# By default, the script will take the Qt headers from /opt/qt/4.6.3, /opt/qt/5.5.1
# and /opt/qt/6.2.1 for Qt4, Qt5 and Qt6 respectively.
#
# Call it from project level as
#
#   ./scripts/mkqtdecl.sh -update        # Qt4
#   ./scripts/mkqtdecl.sh -update -qt5   # Qt5
#   ./scripts/mkqtdecl.sh -update -qt6   # Qt6
# 
# For more options see 
#
#   ./scripts/mkqtdecl.sh -h
#
# 
# Copyright (C) 2006-2024 Matthias Koefferlein
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

dry=0
sync=0
update=0
diff=0
reuse=0
qt="/opt/qt/4.6.3/include"
qt5="/opt/qt/5.5.1/include"
qt6="/opt/qt/6.2.1/include"
inst_dir_common=`pwd`/scripts/mkqtdecl_common
inst_dir4=`pwd`/scripts/mkqtdecl4
inst_dir5=`pwd`/scripts/mkqtdecl5
inst_dir6=`pwd`/scripts/mkqtdecl6
src_dir=`pwd`/src
src_name4=gsiqt/qt4
src_name5=gsiqt/qt5
src_name6=gsiqt/qt6
qt_mods4="QtCore QtGui QtDesigner QtNetwork QtSql QtXml QtUiTools"
qt_mods5="QtCore QtGui QtWidgets QtDesigner QtNetwork QtPrintSupport QtSql QtSvg QtXml QtXmlPatterns QtMultimedia QtUiTools"
qt_mods6="QtCore QtGui QtWidgets QtNetwork QtPrintSupport QtSql QtSvg QtXml QtMultimedia QtUiTools QtCore5Compat"

src_name=$src_name4
inst_dir=$inst_dir4
qt_mods=$qt_mods4

work_dir="mkqtdecl.tmp"

while [ "$1" != "" ]; do

  a="$1"
  shift

  case "$a" in
  -h)
    echo "Produce Qt declaration"
    echo "Usage:"
    echo "  mkqtdecl.sh -update                 Produce and synchronize"
    echo "  mkqtdecl.sh -dry                    Dry run - produce, but don't synchronize"
    echo "  mkqtdecl.sh -sync                   Sync only - don't produce, but synchronize"
    echo "  mkqtdecl.sh -diff                   Show differences - don't produce and don't synchronize"
    echo "  mkqtdecl.sh -qt path_to_include     Use the specified include path"
    echo "  mkqtdecl.sh -qt5                    Use setup for Qt 5.x (use before -qt)"
    echo "  mkqtdecl.sh -qt6                    Use setup for Qt 6.x (use before -qt)"
    echo "  mkqtdecl.sh -reuse                  Don't parse C++ container again"
    exit 0
    ;;
  -dry)
    dry=1
    ;;
  -sync)
    sync=1
    ;;
  -update)
    update=1
    ;;
  -diff)
    diff=1
    ;;
  -reuse)
    reuse=1
    ;;
  -qt)
    qt="$1"
    shift
    ;;
  -qt5)
    qt="$qt5"
    work_dir="mkqtdecl5.tmp"
    inst_dir="$inst_dir5"
    qt_mods="$qt_mods5"
    src_name="$src_name5"
    ;;
  -qt6)
    qt="$qt6"
    work_dir="mkqtdecl6.tmp"
    inst_dir="$inst_dir6"
    qt_mods="$qt_mods6"
    src_name="$src_name6"
    ;;
  *)
    echo "*** ERROR: unknown command option $a"
    exit 1
    ;;
  esac

done

if [ `expr $update + $sync + $dry + $diff` != 1 ]; then
  echo "*** ERROR: either -update, -diff, -dry or -sync must be given"
  exit 1
fi

# Production flags
# -diff   ->  update=0, dry=1
# -dry    ->  update=1, dry=1
# -sync   ->  update=0, dry=0
# -update ->  update=1, dry=0
if [ $diff != 0 ]; then
  dry=1
elif [ $dry != 0 ]; then
  update=1
fi

if [ ! -e $src_dir ]; then 
  echo "*** ERROR: Source directory ($src_dir) not found - run script in repository root"
  exit 1
fi
if [ ! -e $inst_dir ]; then
  echo "*** ERROR: Installation path ($inst_dir) not found - run script in repository root"
  exit 1
fi
if [ ! -e $inst_dir_common ]; then
  echo "*** ERROR: Installation path ($inst_dir_common) not found - run script in repository root"
  exit 1
fi
if ! ruby -v >/dev/null 2>&1; then
  echo "*** ERROR: ruby not installed"
  exit 1
fi
if ! ruby -e "require 'oj'" 2>&1; then
  echo "*** ERROR: ruby gem 'oj' not installed"
  exit 1
fi
if ! ruby -e "require 'treetop'" 2>&1; then
  echo "*** ERROR: ruby gem 'treetop' not installed"
  exit 1
fi

if [ $update != 0 ]; then  

  if [ -e $work_dir ] && [ $reuse == 0 ]; then
    rm -rf $work_dir
  fi

  mkdir -p $work_dir
  cd $work_dir

  cp -R $inst_dir/Qt* .
  for d in Qt*; do
    cd $d
    cp $inst_dir/{mkqtdecl.conf,mkqtdecl.properties,mkqtdecl.events} .
    cp $inst_dir_common/{cpp_parser_classes.rb,cpp_classes.rb,c++.treetop,parse.rb,reader_ext.rb,produce.rb,common.conf} .
    cd ..
  done

  if [ $reuse == 0 ]; then

    for d in $qt_mods; do

      echo "--------------------------------------------------------"
      echo "Parsing $d ..."
      echo ""

      cd $d

      echo "Running gcc preprocessor .."
      # By using -D_GCC_LIMITS_H_ we make the gcc not include constants such as ULONG_MAX which will 
      # remain as such. This way the generated code is more generic.
      gcc -std=c++17 -I$qt -fPIC -D_GCC_LIMITS_H_ -E -o allofqt.x allofqt.cpp 

      echo "Stripping hash lines .."
      egrep -v '^#' <allofqt.x >allofqt.e
      rm allofqt.x

      echo "Parsing and producing db .."
      ./parse.rb -i allofqt.e -o allofqt.db

      cd ..

    done

  fi

  rm -f produced.txt
  touch produced.txt

  # Note the order of the modules is important: first modules get the 
  # classes first
  for d in $qt_mods; do

    echo "--------------------------------------------------------"
    echo "Production on $d ..."
    echo ""

    cd $d

    mkdir -p generated

    echo "Producing code .."
    ./produce.rb -x ../produced.txt -i allofqt.db -m $d

    # mark classes produced here as done
    cat class_list.txt >>../produced.txt

    cd ..

  done

else 
  cd $work_dir 
fi  

for d in $qt_mods; do

  echo "--------------------------------------------------------"
  echo "Processing $d ..."
  echo ""

  cd $d

  if [ ! -e generated ]; then
    echo "*** ERROR: production output ("`pwd`"/generated) does not exist - production failed?"
    exit 1
  fi

  cd generated

  if [ $dry != 0 ]; then
    echo "Synchronizing dry run .."
  else
    echo "Synchronizing .."
  fi

  count=0
  for f in {*.cc,*.h} $d.pri; do
    needs_update=0
    if ! [ -e $src_dir/$src_name/$d/$f ]; then
      echo "# INFO: creating new file $src_dir/$src_name/$f"
      needs_update=1
    elif ! cmp -s $f $src_dir/$src_name/$d/$f; then
      echo "# INFO: out of date: syncing file $src_dir/$src_name/$f"
      needs_update=1
    fi
    if [ $needs_update != 0 ]; then
      count=`expr $count + 1`
      if [ $dry != 0 ]; then
        echo cp $f $src_dir/$src_name/$d/$f
      else
        echo === cp $f $src_dir/$src_name/$d/$f
        cp $f $src_dir/$src_name/$d/$f
      fi
    fi
  done

  if [ $dry != 0 ]; then
    echo "# INFO: $count files to synchronize"
  else
    echo "# INFO: $count files synchronized"
  fi

  cd ..
  cd ..

done
