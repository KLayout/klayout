#! /usr/bin/env python
# -*- coding: utf-8 -*-

#=============================================================================================
# File: "macbuild/makeDMG4mac.py"
#
#  Python script for making a DMG file of KLayout (http://www.klayout.de/index.php) bundles.
#
#  Ref.
#   1) https://el-tramo.be/guides/fancy-dmg/
#=============================================================================================
from __future__ import print_function  # to use print() of Python 3 in Python >= 2.7
import sys
import os
import shutil
import glob
import platform
import optparse
import subprocess

#-------------------------------------------------------------------------------
## To import global dictionaries of different modules and utility functions
#-------------------------------------------------------------------------------
mydir = os.path.dirname(os.path.abspath(__file__))
sys.path.append( mydir + "/macbuild" )
from build4mac_util import *

#-------------------------------------------------------------------------------
## To set global variables including present directory and platform info.
#-------------------------------------------------------------------------------
def SetGlobals():
  global ProjectDir         # project directory where "build.sh" exists
  global Usage              # string on usage
  global Platform           # platform
  global PkgDir             # the package directory where "klayout.app" and "klayout.scripts" exist
  global OpClean            # 'clean' operation
  global OpMake             # 'make' operation
  global DMGSerialNum       # the DMG serial number
  global QtIndification     # Qt identification
  global Version            # KLayout's version
  global DMGFileName        # name of the DMG file
  # auxiliary variables on platform
  global System             # 6-tuple from platform.uname()
  global Node               # - do -
  global Release            # - do -
  global Version            # - do -
  global Machine            # - do -
  global Processor          # - do -
  global Bit                # machine bit-size

  Usage  = "\n"
  Usage += "--------------------------------------------------------------------------------------------------------\n"
  Usage += "<< Usage of 'makeDMG4mac.py' >>\n"
  Usage += "       for making a DMG file of KLayout 0.25 or later on different Apple Mac OSX platforms.\n"
  Usage += "\n"
  Usage += "$ [python] ./makeDMG4mac.py \n"
  Usage += "   option & argument    : descriptions                                               | default value\n"
  Usage += "   ----------------------------------------------------------------------------------+---------------\n"
  Usage += "   <-p|--pkg <dir>>     : package directory created by `build4mac.py` with [-y|-Y]   | `` \n"
  Usage += "                        : like 'qt5.pkg.macos-HighSierra-release'                    | \n"
  Usage += "   <-c|--clean>         : clean the work directory                                   | disabled \n"
  Usage += "   <-m|--make>          : make a DMG file                                            | disabled \n"
  Usage += "                        :   <-c|--clean> and <-m|--make> are mutually exclusive      | \n"
  Usage += "   [-s|--serial <num>]  : DMG serial number                                          | 1 \n"
  Usage += "   [-?|--?]             : print this usage and exit                                  | disabled \n"
  Usage += "--------------------------------------------------------------------------------------------------------\n"

  ProjectDir = os.getcwd()
  (System, Node, Release, Version, Machine, Processor) = platform.uname()

  if not System == "Darwin":
    print("")
    print( "!!! Sorry. Your system <%s> looks like non-Mac" % System, file=sys.stderr )
    print(Usage)
    quit()

  release = int( Release.split(".")[0] ) # take the first of ['14', '5', '0']
  if release == 14:
    Platform = "Yosemite"
  elif release == 15:
    Platform = "ElCapitan"
  elif release == 16:
    Platform = "Sierra"
  elif release == 17:
    Platform = "HighSierra"
  else:
    Platform = ""
    print("")
    print( "!!! Sorry. Unsupported major OS release <%d>" % release, file=sys.stderr )
    print(Usage)
    quit()

  if not Machine == "x86_64":
    print("")
    print( "!!! Sorry. Only x86_64 architecture machine is supported but found <%s>" % Machine, file=sys.stderr )
    print(Usage)
    quit()

  PkgDir         = ""
  OpClean        = False
  OpMake         = False
  DMGSerialNum   = 1
  QtIndification = "Qt593mp" # constant for the time being
  CheckComOnly   = False
  Version        = GetKLayoutVersionFrom( "./version.sh" )
  DMGFileName    = ""

#------------------------------------------------------------------------------
## To check the contents of the package directory
#
# @return True on success; False on failure
#------------------------------------------------------------------------------
def CheckPkgDirectory():
  global ProjectDir
  global Usage
  global PkgDir

  if PkgDir == "":
    print( "! Package directory is not specified", file=sys.stderr )
    print(Usage)
    return False

  if not os.path.isdir(PkgDir):
    print( "! Specified package directory <%s> does not exist" % PkgDir, file=sys.stderr )
    print( "" )
    return False

  os.chdir(PkgDir)
  if not os.path.isdir( "klayout.app" ):
    print( "! The package directory <%s> does not hold <klayout.app> bundle" % PkgDir, file=sys.stderr )
    print( "" )
    os.chdir(ProjectDir)
    return False

  if not os.path.isdir( "klayout.scripts" ):
    print( "! The package directory <%s> does not hold <klayout.scripts> subdirectory" % PkgDir, file=sys.stderr )
    print( "" )
    os.chdir(ProjectDir)
    return False

  os.chdir( "klayout.scripts" )
  if not os.path.isdir( "KLayoutEditor.app" ):
    print( "! The package directory <%s> does not hold <KLayoutEditor.app> bundle" % PkgDir, file=sys.stderr )
    print( "" )
    os.chdir(ProjectDir)
    return False

  if not os.path.isdir( "KLayoutViewer.app" ):
    print( "! The package directory <%s> does not hold <KLayoutViewer.app> bundle" % PkgDir, file=sys.stderr )
    print( "" )
    os.chdir(ProjectDir)
    return False

  os.chdir(ProjectDir)
  return True

#------------------------------------------------------------------------------
## To get command line parameters
#------------------------------------------------------------------------------
def ParseCommandLineArguments():
  global ProjectDir
  global Usage
  global Platform
  global PkgDir
  global OpClean
  global OpMake
  global DMGSerialNum
  global QtIndification
  global Version
  global DMGFileName

  p = optparse.OptionParser( usage=Usage )
  p.add_option( '-p', '--pkg',
                dest='pkg_dir',
                help="the pkg directory" )

  p.add_option( '-c', '--clean',
                action='store_true',
                dest='operation_clean',
                default=False,
                help="clean operation" )

  p.add_option( '-m', '--make',
                action='store_true',
                dest='operation_make',
                default=False,
                help="make operation" )

  p.add_option( '-s', '--serial',
                dest='dmg_serial',
                help="DMG serial number" )

  p.add_option( '-?', '--??',
                action='store_true',
                dest='checkusage',
                default=False,
                help='check usage' )

  p.set_defaults( pkg_dir         = "",
                  operation_clean = False,
                  operation_make  = False,
                  dmg_serial      = "1",
                  checkusage      = False )

  opt, args = p.parse_args()
  if (opt.checkusage):
    print(Usage)
    quit()

  PkgDir         = opt.pkg_dir
  OpClean        = opt.operation_clean
  OpMake         = opt.operation_make
  DMGSerialNum   = int(opt.dmg_serial)
  QtIndification = "Qt593mp"
  DMGFileName    = "klayout-%s-%s-%d-%s.dmg" % (Version, Platform, DMGSerialNum, QtIndification)

  if not CheckPkgDirectory():
    quit()

  if (OpClean and OpMake) or (not OpClean and not OpMake):
    print( "! Specify <-c|--clean> OR <-m|--make>", file=sys.stderr )
    print(Usage)
    quit()

  print( "" )
  print( "### You are going to make <%s> from <%s>" % (DMGFileName, PkgDir) )
  print( "" )

#------------------------------------------------------------------------------
## The main function
#------------------------------------------------------------------------------
def main():
  SetGlobals()
  ParseCommandLineArguments()

#===================================================================================
if __name__ == "__main__":
  main()

#---------------
# End of file
#---------------
