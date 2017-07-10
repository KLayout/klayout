#!/bin/sh -e

inst=$(realpath $(dirname $0))
ld=$(realpath .)

bin="$ld/klayout"

export LD_LIBRARY_PATH=$ld

cd $inst/..

$bin -z -r $inst/create_drc_samples.rb -t -c $inst/klayoutrc_drc_samples
$inst/extract_doc.rb

