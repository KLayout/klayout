#!/bin/sh -e

# deploy-win-ucrt64.sh is an alias for deploy-win-mingw.sh now

inst=$(dirname $(which $0))
$inst/deploy-win-mingw.sh -ucrt $*

