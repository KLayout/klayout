#!/usr/bin/env bash

SCRIPT_NAME=`basename "$0"`
TMP_WHEEL="/tmp/klayout_tempwheel"

display_usage() { 
    echo "This script fixes auditwheel-repaired wheels."
    echo "Caution: This will delete the original wheel." 
    echo -e "\nUsage:\n./${SCRIPT_NAME} [--help|-h] klayout-...-manylinux1_x86_64.whl \n" 
    }

if [[ ( $1 == "--help") || $1 == "-h" ]] 
then 
    display_usage
    exit 0
fi

# Check number of arguments
if [ $# -eq 0 ]; then
    >&2 echo -e "ERROR: No filename supplied\n"
    display_usage
    exit 1
elif [ ! $# -eq 1 ]; then
    >&2 echo -e "ERROR: Too many files supplied. Provide one filename at at time.\n"
    display_usage
    exit 1
fi

# Read wheel file from argument
WHL="$1"

# Check WHL is a valid file
if [[ ! -f "$WHL" ]]; then
    >&2 echo -e "ERROR: $WHL is not a file"
    exit 1
fi

# Convert to absolute path (linux only)
WHL=$(readlink -f $WHL)

# Record old current directory
OLD_PWD=$PWD

# The produced wheel from auditwheel, klayout-*-manylinux1_x86_64.whl, is defective in the following way: dbcore.so etc. have RPATHs reset to `$ORIGIN/.libs`, so we need to move all .so's `lib_*` into `.libs`, as well as `db_plugins`. We also need to change the dist-info/RECORD file paths. This is a bug from auditwheel, it should either have added a new RPATH, $ORIGIN/.libs, where it places libz, libcurl, libexpat, instead of renaming the existing ones, or moved the files to the right place.

# Checking if it was previously patched
if unzip -l $WHL | grep -q 'patched_after_auditwheel_repair'; then
    echo "$(basename $WHL) is already patched. Doing nothing."
    exit 0
fi

# Repair script below
if [[ -d $TMP_WHEEL ]]; then
    rm -rf $TMP_WHEEL
fi
echo "Unpacking $WHL into $TMP_WHEEL"
unzip -q $WHL -d $TMP_WHEEL

cd $TMP_WHEEL/klayout
echo "Moving files: mv lib_* db_plugins .libs/"
mv lib_* db_plugins .libs/ 2>/dev/null
if [ $? -ne 0 ]; then
    >&2 echo "ERROR: lib_*.so or db_plubins not found. Quitting."
    exit 1
fi
cd ../klayout-*.dist-info/
echo "Patching klayout-*.dist-info/RECORD"
sed -i 's/^klayout\/lib_/klayout\/.libs\/lib_/g' RECORD
sed -i 's/^klayout\/db_plugins/klayout\/.libs\/db_plugins/g' RECORD
cd ../
touch $TMP_WHEEL/patched_after_auditwheel_repair
echo "Packing $WHL from $TMP_WHEEL"
rm -f $WHL
zip -rq $WHL ./*
echo "Done. $(basename $WHL) is patched."
# Cleanup (should always execute)
cd $OLD_PWD
