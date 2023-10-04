#!/bin/bash

if [ $# -gt 0 ]; then
	find $1 -type f | grep -E "\.(so|dylib)$"
else
	echo "### Usage of 'findsharelib.sh' ###"
	echo "   To find shared libraries *.[so|dylib] under a <root_dir>"
	echo ""
	echo "   $ findsharelib.sh <root_dir>"
	echo ""
fi
