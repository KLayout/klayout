#!/bin/bash
#-------------------------------------------------------------------------------------
# File: KLayoutEditor.sh
#
# Descriptions:
#  This is to invoke "klayout" with Qt5 distributed as a binary package for Mac
#  in "editor" mode.
#
#  You may specify style and other options as you like by setting
#  "opt_style" and "opt_others" variable in this script.
#-------------------------------------------------------------------------------------

#---------------------------------------------------------
# With "-n" option, multiple instances can be invoked
#---------------------------------------------------------
myKLayout="open -n -a /Applications/klayout.app --args "

#===================================================
# Pass command line parameters to klayout
# vvvvvvvvvv You may edit the block below vvvvvvvvvv
opt_mode="-e"
opt_style="-style=fusion"
opt_others=""
# ^^^^^^^^^^ You may edit the block above ^^^^^^^^^^
#===================================================
options="$opt_mode $opt_style $opt_others"
targetfiles=$@

echo "### Starting KLayout in Editor mode..."
$myKLayout $options $targetfiles &

exit 0

#-------------------
# End of file
#-------------------
