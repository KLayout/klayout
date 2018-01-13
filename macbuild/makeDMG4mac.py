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
  global GenOSName          # generic OS name
  global Platform           # platform
  global PkgDir             # the package directory where "klayout.app" and "klayout.scripts" exist
  global OpClean            # 'clean' operation
  global OpMake             # 'make' operation
  global DMGSerialNum       # the DMG serial number
  global QtIndification     # Qt identification
  global Version            # KLayout's version
  global OccupiedDS         # approx. occupied dist space
  global TargetDMG          # name of the target DMG file
  # Work directories and files
  global TemplateDMG        # unpacked template DMG file
  global WorkDir            # work directory created under PkgDir/
  global WorkDMG            # work DMG file deployed under PkgDir/
  global RootApplications   # reserved directory name for applications
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
  Usage += "   [-q|--qt <ID>]       : ID name of deployed Qt                                     | Qt593mp \n"
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
    GenOSName = "MacOSX"
    Platform  = "Yosemite"
  elif release == 15:
    GenOSName = "MacOSX"
    Platform  = "ElCapitan"
  elif release == 16:
    GenOSName = "macOS"
    Platform  = "Sierra"
  elif release == 17:
    GenOSName = "macOS"
    Platform  = "HighSierra"
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
  QtIndification = "Qt593mp"
  CheckComOnly   = False
  Version        = GetKLayoutVersionFrom( "./version.sh" )
  TargetDMG      = ""

  # Work directories and files
  OccupiedDS       = -1
  TemplateDMG      = "klayout.dmg"    # unpacked template DMG file
                                      # initially stored as 'macbuild/Resouces/klayout.dmg.bz2'
  WorkDir          = "work"           # work directory created under PkgDir/
  WorkDMG          = "work.dmg"       # work DMG file deployed under PkgDir/
  RootApplications = "/Applications"  # reserved directory name for applications

#------------------------------------------------------------------------------
## To check the contents of the package directory
#
# @return on success, positive integer in [MB] of approx. occupied disc space;
#         on failure, -1
#------------------------------------------------------------------------------
def CheckPkgDirectory():

  if PkgDir == "":
    print( "! Package directory is not specified", file=sys.stderr )
    print(Usage)
    return -1

  if not os.path.isdir(PkgDir):
    print( "! Specified package directory <%s> does not exist" % PkgDir, file=sys.stderr )
    print( "" )
    return -1

  os.chdir(PkgDir)
  if not os.path.isdir( "klayout.app" ):
    print( "! The package directory <%s> does not hold <klayout.app> bundle" % PkgDir, file=sys.stderr )
    print( "" )
    os.chdir(ProjectDir)
    return -1

  if not os.path.isdir( "klayout.scripts" ):
    print( "! The package directory <%s> does not hold <klayout.scripts> subdirectory" % PkgDir, file=sys.stderr )
    print( "" )
    os.chdir(ProjectDir)
    return -1

  os.chdir( "klayout.scripts" )
  if not os.path.isdir( "KLayoutEditor.app" ):
    print( "! The package directory <%s> does not hold <KLayoutEditor.app> bundle" % PkgDir, file=sys.stderr )
    print( "" )
    os.chdir(ProjectDir)
    return -1

  if not os.path.isdir( "KLayoutViewer.app" ):
    print( "! The package directory <%s> does not hold <KLayoutViewer.app> bundle" % PkgDir, file=sys.stderr )
    print( "" )
    os.chdir(ProjectDir)
    return -1

  os.chdir(ProjectDir)
  os.chdir(PkgDir)
  size1 = int( os.popen( "du -sm klayout.app" )    .read().strip("\n").split("\t")[0] )
  size2 = int( os.popen( "du -sm klayout.scripts" ).read().strip("\n").split("\t")[0] )
  os.chdir(ProjectDir)
  return size1+size2

#------------------------------------------------------------------------------
## To get command line parameters
#------------------------------------------------------------------------------
def ParseCommandLineArguments():
  global ProjectDir
  global Usage
  global GenOSName
  global Platform
  global PkgDir
  global OpClean
  global OpMake
  global DMGSerialNum
  global QtIndification
  global Version
  global OccupiedDS
  global TargetDMG

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

  p.add_option( '-q', '--qt',
                dest='qt_identification',
                help="Qt's ID" )

  p.add_option( '-s', '--serial',
                dest='dmg_serial',
                help="DMG serial number" )

  p.add_option( '-?', '--??',
                action='store_true',
                dest='checkusage',
                default=False,
                help='check usage' )

  p.set_defaults( pkg_dir           = "",
                  operation_clean   = False,
                  operation_make    = False,
                  qt_identification = "Qt593mp",
                  dmg_serial        = "1",
                  checkusage        = False )

  opt, args = p.parse_args()
  if (opt.checkusage):
    print(Usage)
    quit()

  PkgDir         = opt.pkg_dir
  OpClean        = opt.operation_clean
  OpMake         = opt.operation_make
  QtIndification = opt.qt_identification
  DMGSerialNum   = int(opt.dmg_serial)
  TargetDMG      = "klayout-%s-%s-%s-%d-%s.dmg" % (Version, GenOSName, Platform, DMGSerialNum, QtIndification)

  OccupiedDS = CheckPkgDirectory()
  if not OccupiedDS > 0:
    quit()

  if (OpClean and OpMake) or (not OpClean and not OpMake):
    print( "! Specify <-c|--clean> OR <-m|--make>", file=sys.stderr )
    print(Usage)
    quit()

#------------------------------------------------------------------------------
## Make the target DMG file
#
# @param[in] msg  message to print
#
# @return True on success; False on failure
#------------------------------------------------------------------------------
def MakeTargetDMGFile(msg=""):

  #----------------------------------------------------
  # [1] Print message
  #----------------------------------------------------
  if not msg == "":
    print(msg)

  #--------------------------------------------------------
  # [2] Deploy unpacked template DMG under PkgDir/
  #--------------------------------------------------------
  os.chdir(ProjectDir)
  tmplDMGbz2 = "macbuild/Resources/%s.bz2" % TemplateDMG
  srcDMG     = "macbuild/Resources/%s"     % TemplateDMG
  destDMG    = "%s/%s" % (PkgDir, WorkDMG)
  os.system( "%s -k %s" % ("bunzip2", tmplDMGbz2))
  shutil.copy2( srcDMG, destDMG )
  os.remove( srcDMG )

  #----------------------------------------------------
  # [3] Prepare empty work directory under PkgDir/
  #----------------------------------------------------
  os.chdir(PkgDir)
  if os.path.exists(WorkDir):
    shutil.rmtree(WorkDir)
  os.mkdir(WorkDir)

  #--------------------------------------------------------
  # [4] Attach the work directory to the work DMG
  #--------------------------------------------------------
  os.system( "hdiutil attach %s -noautoopen -quiet -mountpoint %s" % (WorkDMG, WorkDir) )

  #--------------------------------------------------------
  # [5] Populate the work directory
  #--------------------------------------------------------
  os.system( "%s %s %s" % ("cp -Rp", "klayout.app",     WorkDir) )
  os.system( "%s %s %s" % ("cp -Rp", "klayout.scripts", WorkDir) )
  os.symlink( RootApplications, WorkDir + RootApplications )

  #--------------------------------------------------------
  # [6] Detach the work directory
  #
  #     wordDev = /dev/disk10s1 (for example)
  #--------------------------------------------------------
  command = "hdiutil info | grep %s | grep \"/dev/\" | awk '{print $1}'" % WorkDir
  wordDev = os.popen( command ).read().strip('\n')
  if wordDev == "":
    print( "! Failed to identify the file system on which <%s> is mounted" % WorkDir )
    return False
  else:
    os.system( "hdiutil detach %s  -quiet -force" % wordDev )

  #--------------------------------------------------------
  # [7]　Finish up
  #--------------------------------------------------------
  os.system( "rm -Rf %s" % TargetDMG )
  os.system( "hdiutil convert -quiet -format UDRW -imagekey zlib-level=9 -o %s %s" % (TargetDMG, WorkDMG) )
  os.system( "rm -Rf %s" % WorkDir )
  os.system( "rm -Rf %s" % WorkDMG )
  os.system( "rm -Rf %s" % TemplateDMG )
  os.chdir(ProjectDir)

  return True

#------------------------------------------------------------------------------
## Clean up
#
# @param[in] msg  message to print
#
#------------------------------------------------------------------------------
def CleanUp(msg=""):

  #----------------------------------------------------
  # [1] Print message
  #----------------------------------------------------
  if not msg == "":
    print(msg)

  #----------------------------------------------------
  # [2] Clean up
  #----------------------------------------------------
  os.chdir(ProjectDir)
  os.chdir(PkgDir)
  dmgs = glob.glob( "*.dmg" )
  for item in dmgs:
    os.system( "rm -Rf %s" % item )
  os.system( "rm -Rf %s" % WorkDir )
  os.chdir(ProjectDir)

#------------------------------------------------------------------------------
## The main function
#------------------------------------------------------------------------------
def Main():
  SetGlobals()
  ParseCommandLineArguments()
  if OpMake:
    print( "" )
    print( "  ### You are going to make <%s> from <%s>" % (TargetDMG, PkgDir) )
    print( "      KLayout bundles occupy about <%d> [MB] of disc space." % OccupiedDS )
    ok = MakeTargetDMGFile()
    if not ok:
      print( "  !!! Failed to make the target DMG <%s> ..." % TargetDMG, file=sys.stderr )
      print( "", file=sys.stderr )
    else:
      print( "  ### Done" )
      print( "" )
  else:
    print( "" )
    print( "  ### You are going to clean up <%s> directory" % PkgDir )
    CleanUp()
    print( "  ### Done" )
    print( "" )

#===================================================================================
if __name__ == "__main__":
  Main()

#---------------
# End of file
#---------------
