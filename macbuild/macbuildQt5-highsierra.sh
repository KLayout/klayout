#!/bin/bash

#------------------------------------------------------------------------------
# Using Qt 5.9.3 from Mac Ports.
#
# Ruby: OSX native
# Python: OSX native
#------------------------------------------------------------------------------
MacRuby=/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/bin/ruby
MacRubyInc=/System/Library/Frameworks/Ruby.framework/Headers
MacRubyLib=/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/lib/libruby.dylib

MacPython=/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python
MacPythonInc=/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7
MacPythonLib=/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib

MacQMake=/opt/local/libexec/qt5/bin/qmake
MacBinDir=./qt5.build.macos-highsierra
MacBuildDir=./qt5.bin.macos-highsierra
MacBuildLog=macbuildQt5-highsierra.log
#------------------------------------------------------------------------------
function WithRubyPython() {
  ./build.sh                    \
    -release                    \
    -qmake      $MacQMake       \
    -build      $MacBinDir      \
    -bin        $MacBuildDir    \
    -option     -j4             \
    -with-qtbinding             \
    -qt5                        \
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
    -qt5                        \
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
    -qt5                        \
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
    -qt5                        \
    -noruby                     \
    -nopython   2>&1 | tee $MacBuildLog
}
#------------------------------------------------------------------------------
function Usage() {
    echo "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
    echo "This script is for building KLayout for macOS High Sierra"
    echo " with Qt 5.9.3 from MacPorts"
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
