#!/bin/sh -e

bin=""
src=""

self=$(realpath $(which $0))
inst_dir=$(dirname $self)
info=""

while [ "$1" != "" ]; do
  a="$1"
  shift
  if [ "$a" = "-h" ]; then
    echo "./scripts/extract_user_doc -i <branch-info>"
    exit 1
  elif [ "$a" = "-i" ]; then
    info="$1"
    shift
  else
    echo "invalid option $a"
    exit 1
  fi
done

doc_src=./src/lay/lay/doc
. ./version.sh

for qt in 5 4; do

  target_doc=$(pwd)/doc-qt$qt

  rm -rf $target_doc
  mkdir -p $target_doc

  # sanitize the binary dir
  rm -rf bin-release-qt$qt

  export QT_SELECT=$qt
  ./build.sh -qmake qmake -j4 -bin bin-release-qt$qt -build build-release-qt$qt

  for d in programming manual about images; do 
    mkdir -p $target_doc/$d
    for f in $doc_src/$d/*.png; do
      fn=$(basename $f)
      if [ ! -e $target_doc/$d/$fn ] || [ $doc_src/$d/$fn -nt $target_doc/$d/$fn ]; then
        echo "cp $doc_src/$d/$fn $target_doc/$d"
        cp $doc_src/$d/$fn $target_doc/$d 
      fi
    done
  done

  mkdir -p $target_doc/code

  bin=bin-release-qt$qt
  export LD_LIBRARY_PATH=$bin
  export KLAYOUT_PATH=$bin
  export KLAYOUT_HOME=$bin

  rm -f $bin/help-index.xml
  $bin/klayout -rx -b \
    -rd "klayout_version=$KLAYOUT_VERSION" \
    -rd "klayout_version_rev=$KLAYOUT_VERSION_REV" \
    -rd "klayout_version_date=$KLAYOUT_VERSION_DATE" \
    -rd "target_doc=$target_doc" \
    -rd "target_info=$info" \
    -rd "qt=$qt" \
    -r $inst_dir/extract_user_doc.rb

  # just big:
  # mv $bin/help-index.xml $target_doc/help-index.data

done

