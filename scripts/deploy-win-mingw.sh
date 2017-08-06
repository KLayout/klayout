#!/bin/sh -e

if [ "$MSYSTEM" == "MINGW32" ]; then
  arch=win32
  mingw_inst=/mingw32
elif [ "$MSYSTEM" == "MINGW64" ]; then
  arch=win64
  mingw_inst=/mingw64
else
  echo "ERROR: not in mingw32 or mingw64 system."
fi

pwd=$(pwd)
target=$pwd/bin-release-$arch
build=$pwd/build-release-$arch
src=$pwd/src
scripts=$pwd/scripts
python="python3.5m"
ruby="ruby"

makensis=/c/Program\ Files\ \(x86\)/NSIS/makensis.exe

version=$(cat $src/klayout_main/version.h | grep prg_version | sed 's/.*"\(.*\)".*/\1/')
echo "Version is $version"

echo "Running build .."
rm -rf $target
./build.sh -python $python -ruby $ruby -bin $target -build $build -j2

if ! [ -e $target ]; then
  echo "ERROR: Target directory $target not found"
  exit 1
fi

if ! [ -e $target/klayout.exe ]; then
  echo "ERROR: Target directory $target does not contain klayout.exe"
  exit 1
fi

# ----------------------------------------------------------
# Binary dependencies

libs=$(ldd $target/klayout.exe | grep $mingw_inst | sed 's/ *=>.*//' | sort)

for l in $libs; do
  echo "Copying binary installation partial $mingw_inst/$l -> $target/$l .."
  cp $mingw_inst/bin/$l $target/$l
done

# ----------------------------------------------------------
# Ruby dependencies

rm -rf $target/.ruby-paths.txt
echo '# Builds the Python paths.' >$target/.ruby-paths.txt
echo '# KLayout will load the paths listed in this file into sys.path' >>$target/.ruby-paths.txt
echo '# unless $KLAYOUT_PYTHONHOME ist set.' >>$target/.ruby-paths.txt
echo '# Use KLayout EXPRESSIONS syntax to specify a list of file paths.' >>$target/.ruby-paths.txt
echo '[' >>$target/.ruby-paths.txt

first=1
rubys=$($ruby -e 'puts $:' | sort)
for p in $rubys; do 
  p=$(cygpath $p)
  if [[ $p == "$mingw_inst"* ]] && [ -e "$p" ]; then
    rp=${p/"$mingw_inst/"}
    if [ $first == "0" ]; then
      echo "," >>$target/.ruby-paths.txt
    fi
    first=0
    echo -n "  combine(inst_path, '$rp')" >>$target/.ruby-paths.txt
    echo "Copying Ruby installation partial $p -> $target/$rp .."
    rm -rf $target/$rp
    mkdir -p $target/$rp
    rmdir $target/$rp
    cp -vR $p $target/$rp | sed -u 's/.*/echo -n ./' | sh
    echo ""
  fi
done

echo '' >>$target/.ruby-paths.txt
echo ']' >>$target/.ruby-paths.txt

# ----------------------------------------------------------
# Image formats

echo "Installing image format plugins .."
cp -R $mingw_inst/share/qt5/plugins/imageformats $target

# ----------------------------------------------------------
# Python dependencies

rm -rf $target/.python-paths.txt
echo '# Builds the Python paths.' >$target/.python-paths.txt
echo '# KLayout will load the paths listed in this file into sys.path' >>$target/.python-paths.txt
echo '# unless $KLAYOUT_PYTHONHOME ist set.' >>$target/.python-paths.txt
echo '# Use KLayout EXPRESSIONS syntax to specify a list of file paths.' >>$target/.python-paths.txt
echo '[' >>$target/.python-paths.txt

first=1
pythons=$($python -c "import sys; print('\n'.join(sys.path))" | sort)
for p in $pythons; do 
  p=$(cygpath $p)
  if [[ $p == "$mingw_inst"* ]] && [ -e "$p" ]; then
    rp=${p/"$mingw_inst/"}
    if [ $first == "0" ]; then
      echo "," >>$target/.python-paths.txt
    fi
    first=0
    echo -n "  combine(inst_path, '$rp')" >>$target/.python-paths.txt
    echo "Copying Python installation partial $p -> $target/$rp .."
    rm -rf $target/$rp
    mkdir -p $target/$rp
    rmdir $target/$rp
    cp -vR $p $target/$rp | sed -u 's/.*/echo -n ./' | sh
    echo ""
  fi
done

echo '' >>$target/.python-paths.txt
echo ']' >>$target/.python-paths.txt

# ----------------------------------------------------------
# Run NSIS

# TODO: NSIS now supports /nocd with which we would no
# longer require the copy
cp $scripts/klayout-inst.nsis $target
cd $target
NSIS_VERSION=$version NSIS_ARCH=$arch "$makensis" klayout-inst.nsis

