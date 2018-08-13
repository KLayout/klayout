#!/bin/bash
set -e

export PING_SLEEP=30s

error_handler() {
  echo ERROR: There was an error while building.
  tail -500 build.txt
  kill $PING_LOOP_PID
  exit 1
}

trap 'error_handler' ERR

bash -c "while true; do echo -n '.'; sleep $PING_SLEEP; done" &
PING_LOOP_PID=$!

make build PYTHON_VERSION=B37 >> build.txt 2>&1
make deploy PYTHON_VERSION=B37 >> build.txt 2>&1
make test MACOS_VERSION=HighSierra >> build.txt 2>&1
make dropbox-deploy

tail -500 build.txt

echo "build finished"

kill $PING_LOOP_PID

exit 0