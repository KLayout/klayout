#!/bin/bash
#-------------------------------------------------------------------------------------
# File: KLayoutViewer.sh
#
# Descriptions:
#  This is to invoke "klayout" distributed as a binary package for Mac
#  in "viewer" mode.
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
opt_mode="-ne"
opt_style="-style=motif"
opt_others=""
# ^^^^^^^^^^ You may edit the block above ^^^^^^^^^^
#===================================================
options="$opt_mode $opt_style $opt_others"
targetfiles=$@

echo "### Starting KLayout in Viewer mode..."
$myKLayout $options $targetfiles &

exit 0

#-------------------
# End of file
#-------------------
