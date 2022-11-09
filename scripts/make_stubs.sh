#!/bin/bash -e

# Generates LVS and DRC documentation
#
# Run this script from a valid build below the repository root, e.g.
# <repo-root>/build-debug. It needs to have a "pymod" installation in 
# current directory.

inst=$(realpath $(dirname $0))
scripts=${inst}/drc_lvs_doc
ld=$(realpath .)

pymod="$ld/pymod"

export LD_LIBRARY_PATH=$ld
export PYTHONPATH=$pymod

pymod_src=$ld/../src/pymod
if ! [ -e $pymod_src ]; then
  echo "*** ERROR: missing pymod sources ($pymod_src) - did you run the script from the build folder below the source tree?"
  exit 1
fi

if ! [ -e $pymod ]; then
  echo "*** ERROR: missing pymod folder ($pymod) - did you run the script from the build folder?"
  exit 1
fi

python=
for try_python in python python3; do
  if $try_python -c "import klayout.tl" >/dev/null 2>&1; then
    python=$try_python
  fi
done

if [ "$python" = "" ]; then
  echo "*** ERROR: no functional python or pymod installation found."
  exit 1
fi

echo "Generating stubs for tl .."
$python $inst/stubgen.py tl >$pymod_src/distutils_src/klayout/tlcore.pyi

echo "Generating stubs for db .."
$python $inst/stubgen.py db tl >$pymod_src/distutils_src/klayout/dbcore.pyi

echo "Generating stubs for rdb .."
$python $inst/stubgen.py rdb tl,db >$pymod_src/distutils_src/klayout/rdbcore.pyi

echo "Generating stubs for lay .."
$python $inst/stubgen.py lay tl,db,rdb >$pymod_src/distutils_src/klayout/laycore.pyi

echo "Generating stubs for lib .."
$python $inst/stubgen.py lib tl,db >$pymod_src/distutils_src/klayout/libcore.pyi

echo "Done."

