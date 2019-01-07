#!/usr/bin/env bash

SCRIPT_NAME=`basename "$0"`
TMP_WHEEL="$PWD/tmp/klayout_tempwheel"

display_usage() { 
    echo "This script includes external libraries in windows-based wheels."
    echo "Caution: This will delete the original wheel." 
    echo -e "\nUsage:\n./${SCRIPT_NAME} [--help|-h] klayout-...-win_amd64.whl \n" 
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
elif [ ! $# -eq 2 ]; then
    >&2 echo -e "ERROR: Too many files supplied. Provide one filename at at time.\n"
    display_usage
    exit 1
fi

# Read wheel file from argument
WHL="$1"
WHL=$(echo $WHL | sed 's/\\/\//g' | sed 's/://')
KLAYOUT_BITS="$2"
KLAYOUT_BITS=$(echo $KLAYOUT_BITS | sed 's/\\/\//g' | sed 's/://')


# Check WHL is a valid file
if [[ ! -f "$WHL" ]]; then
    >&2 echo -e "ERROR: $WHL is not a file"
    exit 1
fi

# Convert to absolute path (linux only)
WHL=$(readlink -f $WHL)

# Record old current directory
OLD_PWD=$PWD

# Need to include external DLLs (klayout bits) into wheel

# # Checking if it was previously patched
# if unzip -l $WHL | grep -q 'patch_external_libraries_included'; then
#     echo "$(basename $WHL) is already patched. Doing nothing."
#     exit 0
# fi

# Repair script below
if [[ -d $TMP_WHEEL ]]; then
    rm -rf $TMP_WHEEL
else
    mkdir -p $TMP_WHEEL
fi
echo "Unpacking $WHL inside $TMP_WHEEL"
wheel unpack $WHL -d $TMP_WHEEL || exit 1
TMP_WHEEL="$TMP_WHEEL/`ls $TMP_WHEEL`"

cd $TMP_WHEEL/klayout
echo "Copying libraries: libcurl.dll, expat.dll, pthreadVCE2.dll, zlib1.dll"
echo pwd: `pwd`
cp -v $KLAYOUT_BITS/curl/bin/* .
cp -v $KLAYOUT_BITS/expat/bin/* .
cp -v $KLAYOUT_BITS/ptw/bin/* .
cp -v $KLAYOUT_BITS/zlib/bin/* .
# if [ $? -ne 0 ]; then
#     >&2 echo "ERROR: lib not found. Quitting."
#     exit 1
# fi
cd $TMP_WHEEL
# touch $TMP_WHEEL/patch_external_libraries_included
echo "Packing $WHL from $TMP_WHEEL"
rm -f $WHL
wheel pack $TMP_WHEEL -d `dirname $WHL` || exit 1
echo "Done. $(basename $WHL) is patched."
# Cleanup (should always execute)
cd $OLD_PWD
