#!/bin/bash
set -e

export PING_SLEEP=30s

bash -c "while true; do echo -n '.'; sleep $PING_SLEEP; done" &
PING_LOOP_PID=$!

make build >> build.txt 2>&1
make deploy >> build.txt 2>&1
make test >> build.txt 2>&1 || true
make dropbox-deploy

tail -500 build.txt

echo "build finished"

kill $PING_LOOP_PID

exit 0
