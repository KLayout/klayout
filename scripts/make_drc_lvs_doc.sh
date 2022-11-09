#!/bin/bash -e

# Generates LVS and DRC documentation
#
# Run this script from a valid build below the repository root, e.g.
# <repo-root>/build-debug. It needs to have a "klayout" binary in 
# current directory.

inst=$(realpath $(dirname $0))
scripts=${inst}/drc_lvs_doc
ld=$(realpath .)

bin="$ld/klayout"

export LD_LIBRARY_PATH=$ld
export KLAYOUT_HOME=/dev/null

doc_src=../src/doc/doc
if ! [ -e $doc_src ]; then
  echo "*** ERROR: missing doc sources ($doc_src) - did you run the script from the build folder below the source tree?"
  exit 1
fi

if ! [ -e $bin ]; then
  echo "*** ERROR: missing klayout binary ($bin) - did you run the script from the build folder?"
  exit 1
fi

cd $inst/..

$bin -z -r ${scripts}/create_drc_samples.rb -t -c ${scripts}/klayoutrc_drc_samples
${scripts}/extract_doc.rb

