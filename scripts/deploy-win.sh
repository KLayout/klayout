#!/bin/sh -e

inst_dir=$(dirname $(which $0))
if ! [ -e ./build.sh ]; then
  echo "ERROR: build script not found (not in the main directory?)"
  exit 1
fi

pwd=$(pwd)

MSYSTEM=MINGW32 bash --login -c "cd $pwd ; $inst_dir/deploy-win-mingw.sh"
MSYSTEM=MINGW64 bash --login -c "cd $pwd ; $inst_dir/deploy-win-mingw.sh"

