#!/bin/bash -e

tmp=.tmp

rm -rf $tmp
mkdir $tmp
cd $tmp
git clone https://github.com/capnproto/capnproto.git -b v1.0.1 .
cd ..

rm -rf kj capnp

# NOTE: putting the original sources in a folder below the .pro file
# avoids issues with the include path that Qt imposes and which creates
# a name clash with standard headers like "string.h"

mkdir kj
mkdir kj/kj
mkdir kj/kj/parse
mkdir kj/kj/std
mkdir capnp
mkdir capnp/capnp
mkdir capnp/capnp/compat

cp $tmp/{LICENSE,CONTRIBUTORS} kj
cp $tmp/{LICENSE,CONTRIBUTORS} capnp

echo "# DO NOT EDIT!" >kj/kj.pro
echo "# (SEE ../kj.pro)" >>kj/kj.pro
echo "" >>kj/kj.pro
cat kj.pro >>kj/kj.pro

echo "# DO NOT EDIT!" >capnp/capnp.pro
echo "# (SEE ../capnp.pro)" >>capnp/capnp.pro
echo "" >>capnp/capnp.pro
cat capnp.pro >>capnp/capnp.pro

capnp_sources_lite=(
  c++.capnp.c++
  blob.c++
  arena.c++
  layout.c++
  list.c++
  any.c++
  message.c++
  schema.capnp.c++
  stream.capnp.c++
  serialize.c++
  serialize-packed.c++
)

capnp_headers=(
  arena.h
  c++.capnp.h
  common.h
  blob.h
  endian.h
  layout.h
  orphan.h
  list.h
  any.h
  message.h
  capability.h
  membrane.h
  dynamic.h
  schema.h
  schema.capnp.h
  stream.capnp.h
  schema-lite.h
  schema-loader.h
  schema-parser.h
  pretty-print.h
  serialize.h
  serialize-async.h
  serialize-packed.h
  serialize-text.h
  pointer-helpers.h
  generated-header-support.h
  raw-schema.h
)

capnp_compat_headers=(
  std-iterator.h
)

kj_sources_lite=(
  array.c++
  list.c++
  common.c++
  debug.c++
  exception.c++
  io.c++
  memory.c++
  mutex.c++
  string.c++
  source-location.c++
  hash.c++
  table.c++
  thread.c++
  main.c++
  arena.c++
  units.c++
  encoding.c++
  time.c++
)

kj_headers=(
  common.h
  units.h
  memory.h
  refcount.h
  array.h
  list.h
  vector.h
  string.h
  string-tree.h
  source-location.h
  hash.h
  table.h
  map.h
  encoding.h
  exception.h
  debug.h
  arena.h
  io.h
  tuple.h
  one-of.h
  function.h
  mutex.h
  thread.h
  threadlocal.h
  filesystem.h
  time.h
  main.h
  win32-api-version.h
  windows-sanity.h
  miniposix.h
)

kj_parse_headers=(
  common.h
  char.h
)
kj_std_headers=(
  iostream.h
)

for f in ${capnp_sources_lite[@]}; do
  ftarget=${f/.c++/.cc}
  cp $tmp/c++/src/capnp/$f capnp/capnp/$ftarget
done
for f in ${capnp_headers[@]}; do
  cp $tmp/c++/src/capnp/$f capnp/capnp
done
for f in ${capnp_compat_headers[@]}; do
  cp $tmp/c++/src/capnp/compat/$f capnp/capnp/compat
done

pri=capnp/capnp.pri
echo "" >$pri
echo "CAPNP_SOURCES=\\" >>$pri
for f in ${capnp_sources_lite[@]}; do
  ftarget=${f/.c++/.cc}
  echo "  capnp/$ftarget \\" >>$pri
done
echo "" >>$pri
echo "CAPNP_HEADERS=\\" >>$pri
for f in ${capnp_headers[@]}; do
  echo "  capnp/$f \\" >>$pri
done
for f in ${capnp_compat_headers[@]}; do
  echo "  capnp/compat/$f \\" >>$pri
done

for f in ${kj_sources_lite[@]}; do
  ftarget=${f/.c++/.cc}
  cp $tmp/c++/src/kj/$f kj/kj/$ftarget
done
for f in ${kj_headers[@]}; do
  cp $tmp/c++/src/kj/$f kj/kj
done
for f in ${kj_parse_headers[@]}; do
  cp $tmp/c++/src/kj/parse/$f kj/kj/parse
done
for f in ${kj_std_headers[@]}; do
  cp $tmp/c++/src/kj/std/$f kj/kj/std
done

pri=kj/kj.pri
echo "" >$pri
echo "KJ_SOURCES=\\" >>$pri
for f in ${kj_sources_lite[@]}; do
  ftarget=${f/.c++/.cc}
  echo "  kj/$ftarget \\" >>$pri
done
echo "" >>$pri
echo "KJ_HEADERS=\\" >>$pri
for f in ${kj_headers[@]}; do
  echo "  kj/$f \\" >>$pri
done
for f in ${kj_parse_headers[@]}; do
  echo "  kj/parse/$f \\" >>$pri
done
for f in ${kj_std_headers[@]}; do
  echo "  kj/std/$f \\" >>$pri
done

          
