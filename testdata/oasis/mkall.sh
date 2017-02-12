#!/bin/sh -e

LANG=C
export LANG

for t in *.ot; do
  out=`echo $t | sed 's/.ot$/.oas/'`
  echo "$t -> $out"
  ./mkoasis.tcl $t $out
done

