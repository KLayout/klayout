#!/bin/bash

#===============================================================================
# File: "macbuild/macQAT.sh"
#
#  The top Bash script to run "ut_runner" after building KLayout
#  (http://www.klayout.de/index.php) version 0.29.7 or later on different
#  Apple Mac OSX platforms.
#
#  This script must be copied to a "*.macQAT/" directory to run.
#===============================================================================

#----------------------------------------------------------------
# Functions
#----------------------------------------------------------------
function GetPresentDir() {
    path=$1
    array=( `echo $path | tr -s '/' ' '`)
    last_index=`expr ${#array[@]} - 1`
    echo ${array[${last_index}]}
    return 0
}

function HidePrivateDir() {
    if [ -d "../private" ]; then
        ret=$(/bin/mv ../private  ../private.stash)
    fi
}

function ShowPrivateDir() {
    if [ -d "../private.stash" ]; then
        ret=$(/bin/mv ../private.stash  ../private)
    fi
}

#----------------------------------------------------------------
# Variables
#----------------------------------------------------------------
gitSHA1=$(git rev-parse --short HEAD 2>/dev/null)
timestamp=$(date "+%Y_%m%d_%H%M")
presentDir=$(GetPresentDir `pwd`)
logfile=QATest_${gitSHA1}_${timestamp}__${presentDir}.log

#----------------------------------------------------------------
# Environment variables for "ut_runner"
#----------------------------------------------------------------
export TESTSRC=..
export TESTTMP=QATest_${gitSHA1}_${timestamp}__${presentDir}
export DYLD_LIBRARY_PATH=$(pwd):$(pwd)/db_plugins:$(pwd)/lay_plugins:$(pwd)/pymod

#----------------------------------------------------------------
# Environment variables for "ut_runner"
#----------------------------------------------------------------
if [ $# -eq 1 ]; then
    if [ "$1" == "-h" ]; then
        ./ut_runner -h
        exit 0
    fi
    if [ "$1" == "-r" ]; then
        echo "### Dumping the log to" $logfile "..."
        HidePrivateDir
        ./ut_runner -x pymod -c  2>&1 | tee $logfile
        ShowPrivateDir
        exit 0
    else
        echo ""
        echo " Git SHA1   = ${gitSHA1}"
        echo " Time stamp = ${timestamp}"
        echo " Log file   = ${logfile}"
        echo " Usage:"
        echo "  ./macQAT.sh -h: to get the help of 'ut_runner'"
        echo "  ./macQAT.sh -r: to run the tests with '-c' option: continues after an error"
        echo ""
        exit 0
    fi
else
    echo ""
    echo " Git SHA1   = ${gitSHA1}"
    echo " Time stamp = ${timestamp}"
    echo " Log file   = ${logfile}"
    echo " Usage:"
    echo "  ./macQAT.sh -h: to get the help of 'ut_runner'"
    echo "  ./macQAT.sh -r: to run the tests with '-c' option: continues after an error"
    echo ""
    exit 0
fi

#--------------
# End of File
#--------------
