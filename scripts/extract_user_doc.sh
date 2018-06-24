#!/bin/sh -e

bin=""
src=""

self=$(realpath $(which $0))
inst_dir=$(dirname $self)

while [ "$1" != "" ]; do
  a="$1"
  shift
  if [ "$a" = "-h" ]; then
    echo "extract_user_doc.sh"
    echo "  ./scripts/extract_user_doc"
    exit 1
  else
    echo "invalid option $a"
    exit 1
  fi
done

doc_src=./src/lay/lay/doc

for qt in 5 4; do

  target_doc=./doc-qt$qt

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

  cd bin-release-qt$qt
  export LD_LIBRARY_PATH=.
  export KLAYOUT_PATH=.
  export KLAYOUT_HOME=.

  rm -f ./help-index.xml
  ./klayout -rx -z -rd "target_doc=$target_doc" -rd "qt=$qt" -r $inst_dir/extract_user_doc.rb

  mv ./help-index.xml $target_doc/help-index.data

done

