#!/usr/bin/env bash

SCRIPT_NAME=`basename "$0"`

display_usage() { 
    echo "This script fixes auditwheel-repaired wheels."
    echo "Caution: This will delete the original wheel." 
    echo -e "\nUsage:\n./${SCRIPT_NAME} klayout-...-manylinux1_x86_64.whl \n" 
    }

if [[ ( $1 == "--help") || $1 == "-h" ]] 
then 
    display_usage
    exit 0
fi

# Check number of arguments
if [ $# -eq 0 ]; then
    echo -e "No filename supplied\n"
    display_usage
    exit 1
elif [ ! $# -eq 1 ]; then
    echo -e "Too many files supplied. Provide one filename at at time.\n"
    display_usage
    exit 1
fi

# Read wheel file from argument
WHL="$1"

# Check WHL is a valid file
if [[ ! -f "$WHL" ]]; then
    echo -e "$WHL is not a file"
    exit 1
fi

# Convert to absolute path (linux only)
WHL=$(readlink -f $WHL)

# Record old current directory
OLD_PWD=$PWD

# The produced wheel from auditwheel, klayout-*-manylinux1_x86_64.whl, is defective in the following way: dbcore.so etc. have RPATHs reset to `$ORIGIN/.libs`, so we need to move all .so's `lib_*` into `.libs`, as well as `db_plugins`. We also need to change the dist-info/RECORD file paths. This is a bug from auditwheel, it should either have added a new RPATH, $ORIGIN/.libs, where it places libz, libcurl, libexpat, instead of renaming the existing ones, or moved the files to the right place.

# Repair script below
echo $WHL
echo $OLD_PWD

unzip $WHL -d /tmp/tempwheel
cd /tmp/tempwheel/klayout
mv lib_* db_plugins .libs/
cd ../klayout-*.dist-info/
sed -i 's/^klayout\/lib_/klayout\/.libs\/lib_/g' RECORD
sed -i 's/^klayout\/db_plugins/klayout\/.libs\/db_plugins/g' RECORD
cd ../
rm -f $WHL
zip -rq $WHL ./*

# Cleanup (should always execute)
cd $OLD_PWD
rm -rf /tmp/tempwheel
