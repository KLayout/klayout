#!/bin/bash -e

qt=()
for qm in qmake qmake4 qmake5 qmake6; do
  if sh -c "$qm -v" 2>/dev/null >/dev/null; then
    qt_version=$($qm -v | grep 'Qt version' | sed 's/.*Qt version *\([0-9]\)\..*/\1/')
    if [ "$qt_version" != "" ]; then
      echo "Found qmake for Qt$qt_version: $qm"
      qt[$qt_version]=$qm  
    fi
  fi
done

qmake=qmake

inst_dir_common=$(pwd)/scripts/mkqtdecl_common
inst_dir4=$(pwd)/scripts/mkqtdecl4
inst_dir5=$(pwd)/scripts/mkqtdecl5
inst_dir6=$(pwd)/scripts/mkqtdecl6

inst_dir=$inst_dir4

work_dir="mkqtdecl.tmp"

while [ "$1" != "" ]; do

  a="$1"
  shift

  case "$a" in
  -h)
    echo "Update event and property tables"
    echo "Usage:"
    echo "  mkqtdecl_update_tables.sh                     Update tables for Qt4"
    echo "  mkqtdecl_update_tables.sh -qt5                Update tables for Qt5"
    echo "  mkqtdecl_update_tables.sh -qt6                Update tables for Qt6"
    echo "  mkqtdecl_update_tables.sh -qt <qmake-path>    Update tables for specific Qt installation"
    exit 0
    ;;
  -qt)
    qmake="$1"
    shift
    ;;
  -qt5)
    qmake="${qt[5]}"
    if [ "$qmake" == "" ]; then
      echo "*** ERROR: Could not find qmake for Qt5"
      exit 1
    fi
    work_dir="mkqtdecl5.tmp"
    inst_dir="$inst_dir5"
    ;;
  -qt6)
    qmake="${qt[6]}"
    if [ "$qmake" == "" ]; then
      echo "*** ERROR: Could not find qmake for Qt6"
      exit 1
    fi
    work_dir="mkqtdecl6.tmp"
    inst_dir="$inst_dir6"
    ;;
  *)
    echo "*** ERROR: unknown command option $a"
    exit 1
    ;;
  esac

done

if ! [ -e build.sh ]; then
  echo "*** ERROR: could not find build script in current directy - did you start this script from top level?"
  exit 1
fi

mkdir -p $work_dir

bin=$work_dir/bin-update-tables
build=$work_dir/build-update-tables
log=$work_dir/build-update-table.log

echo "Building in $build (log in $log) .."

./build.sh -qmake $qmake -nopython -j8 -release -prefix $(pwd)/$bin -bin $bin -build $build >$log 2>&1

echo "Extracting tables .."

export LD_LIBRARY_PATH=$bin
echo "[1] for properties .."
$bin/klayout -b -r $inst_dir_common/mkqtdecl_extract_props.rb -rd output=$inst_dir/mkqtdecl.properties
echo "[2] for signals .."
$bin/klayout -b -r $inst_dir_common/mkqtdecl_extract_signals.rb -rd output=$inst_dir/mkqtdecl.events

echo "Done."

