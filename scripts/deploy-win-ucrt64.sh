#!/bin/sh -e

# deploy-win-ucrt64.sh is an alias for deploy-win-mingw.sh now

export PYTHONPATH=/ucrt64/bin:$PYTHONPATH

inst=$(dirname $(which $0))
$inst/deploy-win-mingw.sh -ucrt $*

