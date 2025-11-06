#!/bin/bash -e

rm -rf capnp_src
rm *.cc *.h

tmp=.tmp
rm -rf $tmp
mkdir $tmp
cd $tmp

git clone ssh://git@codeberg.org/klayoutmatthias/lstream.git .

cp capnp/*.capnp ../capnp_src
cp db_plugin/*{cc,h} ..
cd ..

./capnp_compile.sh

