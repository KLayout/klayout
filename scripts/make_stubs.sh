#!/bin/bash -e

# Generates pyi stubs
#
# Run this script from a valid build below the repository root, e.g.
# <repo-root>/build-debug. It needs to have a "pymod" installation in
# current directory.

inst=$(realpath $(dirname $0))

## Detect if klayout pymod libraries are installed
python=
for try_python in python python3; do
  if $try_python -c "import klayout.tl" >/dev/null 2>&1; then
    python=$try_python
  fi
done

if [ "$python" = "" ]; then
  echo "*** Searching for pymod..."

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

pyi_srcdir="$inst/../src/pymod/distutils_src/klayout"

echo "Generating stubs for tl .."
$python $inst/stubgen.py tl >$pyi_srcdir/tlcore.pyi

echo "Generating stubs for db .."
$python $inst/stubgen.py db tl,lay,rdb >$pyi_srcdir/dbcore.pyi

echo "Generating stubs for rdb .."
$python $inst/stubgen.py rdb tl,db >$pyi_srcdir/rdbcore.pyi

echo "Generating stubs for lay .."
$python $inst/stubgen.py lay tl,db,rdb >$pyi_srcdir/laycore.pyi

echo "Generating stubs for lib .."
$python $inst/stubgen.py lib tl,db >$pyi_srcdir/libcore.pyi

echo "Done."
