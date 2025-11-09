#!/bin/bash -e

capnp_version=$(capnp --version)
if [ "$capnp_version" != "Cap'n Proto version 1.0.1" ]; then
  echo "ERROR: needs capnp version 1.0.1, got '$capnp_version'"
  exit 1
fi
  
src=(
  cell.capnp
  geometry.capnp
  header.capnp
  layoutView.capnp
  library.capnp
  metaData.capnp
  metaDataView.capnp
  propertySet.capnp
  repetition.capnp
  variant.capnp
)

srcdir=$(pwd)/capnp_src

dest=$(pwd)/capnp
rm -rf $dest
mkdir $dest

cd $srcdir
for f in ${src[@]}; do
  echo "Compiling $f .."
	capnp compile -o /usr/bin/capnpc-c++:$dest --src-prefix $dest -I $srcdir $f
  mv $dest/$f.c++ $dest/$f.cc
done

pri=$dest/capnp.pri
echo "" >$pri
echo "HEADERS=\\" >>$pri
for f in ${src[@]}; do
  echo "  capnp/$f.h \\" >>$pri
done
echo "" >>$pri

echo "SOURCES=\\" >>$pri
for f in ${src[@]}; do
  echo "  capnp/$f.cc \\" >>$pri
done
echo "" >>$pri

