#!/bin/bash
# File: macbuild/cleanQAT.sh
#
# Task: cleanup qt*.macQAT directories

for d in *macQAT*; do
    if [ -d "$d" ]; then
        echo "Processing: $d"
        (
            cd "$d" || exit 1
            \rm -rf QATest*
        )
    fi
done
