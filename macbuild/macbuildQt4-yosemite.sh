#!/bin/bash
#------------------------------------------------------------------------------
# Using Qt 4.8.7 from Mac Ports.
#
# Ruby: OSX native
# Python: OSX native
#------------------------------------------------------------------------------
MacRuby=/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/bin/ruby
MacRubyInc=/System/Library/Frameworks/Ruby.framework/Headers
MacRubyLib=/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/lib/libruby.dylib

MacPython=/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python
MacPythonInc=/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7
MacPythonLib=/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib

MacQMake=/opt/local/libexec/qt4/bin/qmake
MacBinDir=./qt4.build.macos-yosemite
MacBuildDir=./qt4.bin.macos-yosemite
MacBuildLog=macbuildQt4-yosemite.log
#------------------------------------------------------------------------------
function WithRubyPython() {
  ./build.sh                    \
    -release                    \
    -qmake      $MacQMake       \
    -build      $MacBinDir      \
    -bin        $MacBuildDir    \
    -option     -j4             \
    -with-qtbinding             \
    -qt4                        \
    -ruby       $MacRuby        \
    -rbinc      $MacRubyInc     \
    -rblib      $MacRubyLib     \
    -python     $MacPython      \
    -pyinc      $MacPythonInc   \
    -pylib      $MacPythonLib 2>&1 | tee $MacBuildLog
}
#------------------------------------------------------------------------------
function WithRuby() {
  ./build.sh                    \
    -release                    \
    -qmake      $MacQMake       \
    -build      $MacBinDir      \
    -bin        $MacBuildDir    \
    -option     -j4             \
    -with-qtbinding             \
    -qt4                        \
    -ruby       $MacRuby        \
    -rbinc      $MacRubyInc     \
    -rblib      $MacRubyLib     \
    -nopython   2>&1 | tee $MacBuildLog
}
#------------------------------------------------------------------------------
function WithPython() {
  ./build.sh                    \
    -release                    \
    -qmake      $MacQMake       \
    -build      $MacBinDir      \
    -bin        $MacBuildDir    \
    -option     -j4             \
    -with-qtbinding             \
    -qt4                        \
    -noruby                     \
    -python     $MacPython      \
    -pyinc      $MacPythonInc   \
    -pylib      $MacPythonLib 2>&1 | tee $MacBuildLog
}
#------------------------------------------------------------------------------
function WithoutRubytPython() {
  ./build.sh                    \
    -release                    \
    -qmake      $MacQMake       \
    -build      $MacBinDir      \
    -bin        $MacBuildDir    \
    -option     -j4             \
    -with-qtbinding             \
    -qt4                        \
    -noruby                     \
    -nopython   2>&1 | tee $MacBuildLog
}
#------------------------------------------------------------------------------
function Usage() {
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo "This script is for building KLayout for Mac OSX Yosemite"
    echo " with Qt 4.8.7 from MacPorts"
    echo ""
    echo "USAGE:"
    echo "  $0  < 0 | 1 | 2 | 3 >"
    echo "        0: support neither Ruby nor Python"
    echo "        1: support both Ruby and Python"
    echo "        2: support Ruby only"
    echo "        3: support Python only"
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo ""
}
#------------------------------------------------------------------------------
if [ $# -ne 1 ]; then
    Usage
    exit
else
    if [ "$1" = "0" ]; then
        WithoutRubytPython
    elif [ "$1" = "1" ]; then
        WithRubyPython
    elif [ "$1" = "2" ]; then
        WithRuby
    elif [ "$1" = "3" ]; then
        WithPython
    else
        Usage
        exit
    fi
fi
#----------
# EOF
#----------
