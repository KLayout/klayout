#!/bin/bash
set -e

export PING_SLEEP=30s

bash -c "while true; do echo -n '.'; sleep $PING_SLEEP; done" &
PING_LOOP_PID=$!

touch build.txt

echo "build"
make build >> build.txt 2>&1 || tail -500 build.txt
echo "deploy"
make deploy >> build.txt 2>&1 || tail -500 build.txt
echo "test"
make test >> build.txt 2>&1 || tail -500 build.txt
echo "dropbox-deploy"
make dropbox-deploy

echo "build finished"

kill $PING_LOOP_PID

exit 0
