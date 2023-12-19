#!/bin/sh -e

# Specify the Python interpreter to use.
# This is the command executed for the Python interpreter that is going
# to be included in KLayout.

python="python3"

# Specify the Ruby interpreter to use.
# This is the command executed for the Ruby interpreter that is going
# to be included in KLayout.

ruby="ruby"

# Specify the path to the NSIS compiler

makensis=/c/Program\ Files\ \(x86\)/NSIS/makensis.exe

# ---------------------------------------------------
# General initialization

if ! [ -e ./build.sh ]; then
  echo "ERROR: build script not found (not in the main directory?)"
  exit 1
fi

pwd=$(pwd)

args=""
suffix=""

while [ "$1" != "" ]; do
  if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Runs the Windows build include installer generation."
    echo ""
    echo "Run this script from the root directory."
    echo ""
    echo "Usage:"
    echo "  scripts/deploy-win-ucrt64.sh <options>"
    echo ""
    echo "Options:"
    echo "  -s <suffix>  Binary suffix"
    echo ""
    echo "Only 64 bit builds are performed."
    exit 0
  elif [ "$1" = "-s" ]; then
    shift
    suffix="-$1"
  else
    args="$args $1"
  fi
  shift
done

# ---------------------------------------------------
# Bootstrap script
# This branch will fork to the actual builds for win32 and win64

if [ "$KLAYOUT_BUILD_IN_PROGRESS" == "" ]; then

  self=$(which $0)

  export KLAYOUT_BUILD_IN_PROGRESS=1
  export KLAYOUT_BUILD_ARGS="$args"
  export KLAYOUT_BUILD_SUFFIX="$suffix"


  # Run ourself in UCRT64 system for the win64 build
  MSYSTEM=UCRT64 bash --login -c "cd $pwd ; $self"

  exit 0

fi

# ---------------------------------------------------
# Actual build branch

if [ "$MSYSTEM" == "UCRT64" ]; then

  arch=win64-ucrt
  ucrt_inst=/ucrt64

  shopt -s nullglob
  ucrt_vssdk=(/c/Program\ Files\ \(x86\)/Windows\ Kits/10/Redist/10.0.*)
  shopt -u nullglob
  ucrt_vssdk=${ucrt_vssdk[0]}
  if [ "$ucrt_vssdk" = "" ]; then
    echo "ERROR: ucrt64 DLLs not found"
    exit 1
  fi
  ucrt_vssdk=$(cygpath -w "$ucrt_vssdk")

else
  echo "ERROR: not in ucrt64 system."
fi

target=$pwd/bin-release-$arch
build=$pwd/build-release-$arch
src=$pwd/src
scripts=$pwd/scripts
# Update in NSIS script too:
plugins="audio generic iconengines imageformats platforms printsupport sqldrivers styles"

# read the current version
. ./version.sh

echo "------------------------------------------------------------------"
echo "Running build for architecture $arch .."
echo ""
echo "  target     = $target"
echo "  build      = $build"
echo "  version    = $KLAYOUT_VERSION"
echo "  build args = $KLAYOUT_BUILD_ARGS"
echo "  suffix     = $KLAYOUT_BUILD_SUFFIX"
echo ""
echo "  UCRT libs  = $ucrt_vssdk"
echo ""

rm -rf $target
./build.sh -python $python -ruby $ruby -bin $target -build $build -j2$KLAYOUT_BUILD_ARGS

if ! [ -e $target ]; then
  echo "ERROR: Target directory $target not found"
  exit 1
fi

if ! [ -e $target/klayout.exe ]; then
  echo "ERROR: Target directory $target does not contain klayout.exe"
  exit 1
fi

# ----------------------------------------------------------
# cert.pem

echo "Installing cert.pem .."

cp $ucrt_inst/etc/ssl/cert.pem $target

# ----------------------------------------------------------
# Plugins

echo "Installing plugins .."
for p in $plugins; do
  cp -R $ucrt_inst/share/qt5/plugins/$p $target
  # remove the debug versions - otherwise they pull in the debug Qt libs
  shopt -s nullglob
  rm -f $target/$p/*d.dll $target/$p/*.dll.debug
  shopt -u nullglob
done

# ----------------------------------------------------------
# Ruby dependencies

rubys=$($ruby -e 'puts $:' | sort | awk '{print $1}')

rm -rf $target/.ruby-paths.txt
echo '# Builds the Ruby paths.' >$target/.ruby-paths.txt
echo '# KLayout will load the paths listed in this file into $0' >>$target/.ruby-paths.txt
echo '# Use KLayout EXPRESSIONS syntax to specify a list of file paths.' >>$target/.ruby-paths.txt
echo '[' >>$target/.ruby-paths.txt

first=1
for p in $rubys; do 
  p=$(cygpath $p)
  if [[ $p == "$ucrt_inst"* ]] && [ -e "$p" ]; then
    rp=${p/"$ucrt_inst/"}
    # Apparently adding the paths to the interpreter isn't required - 
    # Ruby can figure out it's own paths
    # if [ $first == "0" ]; then
    #   echo "," >>$target/.ruby-paths.txt
    # fi
    # first=0
    # echo -n "  combine(inst_path, '$rp')" >>$target/.ruby-paths.txt
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

# don't forget the gem directory (specifications and gems)
p=$(ruby -e 'puts Gem::dir')
p=$(cygpath $p)
if [[ $p == "$ucrt_inst"* ]] && [ -e "$p" ]; then
  rp=${p/"$ucrt_inst/"}
  echo "Copying Ruby gems $p -> $target/$rp .."
  rm -rf $target/$rp
  mkdir -p $target/$rp
  rmdir $target/$rp
  cp -vR $p $target/$rp | sed -u 's/.*/echo -n ./' | sh
  echo ""
fi

# ----------------------------------------------------------
# Python dependencies

pythons=$($python -c "import sys; print('\n'.join(sys.path))" | sort)

rm -rf $target/.python-paths.txt
echo '# Builds the Python paths.' >$target/.python-paths.txt
echo '# KLayout will load the paths listed in this file into sys.path' >>$target/.python-paths.txt
echo '# unless $KLAYOUT_PYTHONHOME ist set.' >>$target/.python-paths.txt
echo '# Use KLayout EXPRESSIONS syntax to specify a list of file paths.' >>$target/.python-paths.txt
echo '[' >>$target/.python-paths.txt

first=1
for p in $pythons; do 
  p=$(cygpath $p)
  rp=""
  if [[ $p == "$ucrt_inst"* ]] && [ -e "$p" ]; then
    rp=${p/"$ucrt_inst/"}
  fi
  # NOTE: "bin" is in the path sometimes and will pollute our installation, so we skip it
  if [ "$rp" != "" ] && [ "$rp" != "bin" ]; then
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
# Binary dependencies

pushd $target

new_libs=$(find . -name "*.dll" -or -name "*.pyd" -or -name "*.so")

while [ "$new_libs" != "" ]; do

  echo "Analyzing dependencies of $new_libs .."

  # Analyze the dependencies of our components and add the corresponding libraries from $ucrt_inst/bin
  tmp_libs=.tmp-libs.txt
  rm -f $tmp_libs
  echo "" >$tmp_libs
  for l in $new_libs; do
    echo -n "."
    objdump -p $l | grep "DLL Name:" | sed 's/.*DLL Name: *//' >>$tmp_libs
  done
  echo ""
  libs=$(cat $tmp_libs | sort -u)
  rm -f $tmp_libs
  new_libs=""

  for l in $libs; do
    if [ -e $ucrt_inst/bin/$l ] && ! [ -e $l ]; then
      echo "Copying binary installation partial $ucrt_inst/bin/$l -> $l"
      cp $ucrt_inst/bin/$l $l
      new_libs="$new_libs $l"
    elif [ -e "${ucrt_vssdk}/$l" ] && ! [ -e $l ]; then
      echo "Copying binary installation partial $ucrt_inst/bin/$l -> $l"
      cp "${ucrt_vssdk}/${l}" "$l"
      new_libs="$new_libs $l"
    fi  
  done

done

popd

# ----------------------------------------------------------
# Run NSIS

# TODO: NSIS now supports /nocd with which we would no
# longer require the copy
cp $scripts/klayout-inst.nsis $target
cd $target
NSIS_VERSION=$KLAYOUT_VERSION NSIS_ARCH=$arch$KLAYOUT_BUILD_SUFFIX "$makensis" klayout-inst.nsis

# ----------------------------------------------------------
# Produce the .zip file

zipname="klayout-$KLAYOUT_VERSION-$arch$KLAYOUT_BUILD_SUFFIX"

echo "Making .zip file $zipname.zip .."

rm -rf $zipname $zipname.zip
mkdir $zipname
cp -Rv *.dll cert.pem .*-paths.txt db_plugins lay_plugins $plugins lib $zipname | sed -u 's/.*/echo -n ./' | sh
cp klayout.exe $zipname/klayout_app.exe
cp klayout.exe $zipname/klayout_vo_app.exe
echo ""

zip -r $zipname.zip $zipname
rm -rf $zipname

