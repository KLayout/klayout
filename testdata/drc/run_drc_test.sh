#!/bin/sh -e

# Must be run in the build directory where klayout and strcmp are located.

inst=$(dirname $0)

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

make klayout 

for t in drctest drctest2; do

  echo "-----------------------------------------------"
  echo "Run drctest.drc ..."

  ./klayout -zz -r $inst/$t.drc

  strmcmp ${t}_out.gds $inst/${t}_au.oas.gz

done

