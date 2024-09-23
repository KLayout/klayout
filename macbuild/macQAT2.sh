#!/bin/bash

#===============================================================================
# File: "macbuild/macQAT2.sh"
#
#  The top Bash script to run "ut_runner" after building KLayout
#  (http://www.klayout.de/index.php) version 0.29.7 or later on different
#  Apple Mac OSX platforms.
#
#  This script must be copied to a directory that can be found in $PATH.
#===============================================================================

prjdir=`pwd`

if [ $# -ge 2 ]; then
	first_arg=$1
	shift
	remaining_args=("$@")
	echo "#########################"
	echo "#  Starging macQAT2.sh  #"
	echo "#########################"
	echo "   Current Dir: $prjdir"
	echo "     First Arg: $first_arg"
	echo "Remaining Args: ${remaining_args[@]}"
	echo ""
	export TESTSRC=../
	export TESTTMP=$first_arg
	export DYLD_LIBRARY_PATH=$prjdir:$prjdir/db_plugins:$prjdir/lay_plugins
	./ut_runner ${remaining_args[@]}
else
	echo "Usage: -------------------------------------------------"
	echo "  $ macQAT2.sh <test_dir> <args to ./ut_runner>"
	echo "    ex. $ change directory to a *.macQAT/"
	echo "        $ macQAT2.sh  __Homebrew  tlGitTests"
	echo "--------------------------------------------------------"
fi

#--------------
# End of File
#--------------
