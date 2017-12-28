#! /usr/bin/env python
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac.py"
#
#  The top Python script for building KLayout (http://www.klayout.de/index.php)
#  version 0.25 or later on different Apple Mac OSX platforms.
#===============================================================================
from __future__ import print_function  # to use print() of Python 3 in Python >= 2.7
import sys
import os
import shutil
import glob
import platform
import optparse
import subprocess

#-------------------------------------------------------------------------------
## To import global dictionaries of different modules
#-------------------------------------------------------------------------------
mydir = os.path.dirname(os.path.abspath(__file__))
sys.path.append( mydir + "/macbuild" )
from build4mac_env  import *
from build4mac_util import *

#-------------------------------------------------------------------------------
## To set global variables including present directory and platform info.
#-------------------------------------------------------------------------------
def SetGlobals():
  global ProjectDir         # project directory where "build.sh" exists
  global Usage              # string on usage
  global BuildBash          # the main build Bash script
  global Platform           # platform
  global ModuleQt           # Qt module to be used
  global ModuleRuby         # Ruby module to be used
  global ModulePython       # Python module to be used
  global NoQtBindings       # True if not creating Qt bindings for Ruby scripts
  global MakeOptions        # options passed to `make`
  global DebugMode          # True if debug mode build
  global CheckComOnly       # True if only for checking the command line parameters to "build.sh"
  global Deployment         # True if deploying the binaries for a package
  # auxiliary variables on platform
  global System             # 6-tuple from platform.uname()
  global Node               # - do -
  global Release            # - do -
  global Version            # - do -
  global Machine            # - do -
  global Processor          # - do -
  global Bit                # machine bit-size

  Usage  = "\n"
  Usage += "---------------------------------------------------------------------------------------------\n"
  Usage += "<< Usage of 'build4mac.py' >>\n"
  Usage += "       for building KLayout 0.25 or later on different Apple Mac OSX platforms.\n"
  Usage += "\n"
  Usage += "$ [python] build4mac.py \n"
  Usage += "   option & argument    : comment on option if any                        | default value\n"
  Usage += "   -----------------------------------------------------------------------+---------------\n"
  Usage += "                        : * key type names below are case insensitive *   | \n"
  Usage += "                        :   'nil' = not to support the script language    | \n"
  Usage += "                        :   'Sys' = using the OS standard script language | \n"
  Usage += "   [-q|--qt <type>]     : type=['Qt4MacPorts', 'Qt5MacPorts']             | qt5macports \n"
  Usage += "   [-r|--ruby <type>]   : type=['nil', 'Sys', 'RubySource']               | sys \n"
  Usage += "   [-p|--python <type>] : type=['nil', 'Sys', 'Anaconda27', 'Anaconda36'] | sys \n"
  Usage += "   [-n|--noqtbinding]   : don't create Qt bindings for ruby scripts       | disabled \n"
  Usage += "   [-m|--make <option>] : option passed to 'make'                         | -j4 \n"
  Usage += "   [-d|--debug]         : enable debug mode build                         | disabled \n"
  Usage += "   [-c|--checkcom]      : check command line and exit without building    | disabled \n"
  Usage += "   [-y|--deploy]        : deploy built binaries with depending libraries  | disabled \n"
  Usage += "                        : ! After confirmation of successful build of     | \n"
  Usage += "                        :   KLayout, rerun this script with BOTH:         | \n"
  Usage += "                        :     1) the same options used for building AND   | \n"
  Usage += "                        :     2) [-y|--deploy]                            | \n"
  Usage += "   [-?|--?]             : print this usage and exit                       | disabled \n"
  Usage += "---------------------------------------------------------------------------------------------\n"

  ProjectDir = os.getcwd()
  BuildBash  = "./build.sh"
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

  # default modules
  ModuleQt = "Qt5MacPorts"
  if Platform == "Yosemite":
    ModuleRuby   = "RubyYosemite"
    ModulePython = "PythonYosemite"
  elif Platform == "ElCapitan":
    ModuleRuby   = "RubyElCapitan"
    ModulePytyon = "PythonElCapitan"
  elif Platform == "Sierra":
    ModuleRuby   = "RubySierra"
    ModulePython = "PythonSierra"
  elif Platform == "HighSierra":
    ModuleRuby   = "RubyHighSierra"
    ModulePython = "PythonHighSierra"
  else:
    ModuleRuby   = "nil"
    ModulePython = "nil"

  NoQtBindings = False
  MakeOptions  = "-j4"
  DebugMode    = False
  CheckComOnly = False
  Deployment   = False

#------------------------------------------------------------------------------
## To get command line parameters
#------------------------------------------------------------------------------
def ParseCommandLineArguments():
  global Usage
  global Platform
  global ModuleQt
  global ModuleRuby
  global ModulePython
  global NoQtBindings
  global MakeOptions
  global DebugMode
  global CheckComOnly
  global Deployment

  p = optparse.OptionParser( usage=Usage )
  p.add_option( '-q', '--qt',
                dest='type_qt',
                help="Qt type=['Qt4MacPorts', 'Qt5MacPorts']" )

  p.add_option( '-r', '--ruby',
                dest='type_ruby',
                help="Ruby type=['nil', 'Sys', 'RubySource']" )

  p.add_option( '-p', '--python',
                dest='type_python',
                help="Python type=['nil', 'Sys', 'Anaconda27', 'Anaconda36']" )

  p.add_option( '-n', '--noqtbinding',
                action='store_true',
                dest='no_qt_binding',
                default=False,
                help="do not create Qt bindings for Ruby scripts" )

  p.add_option( '-m', '--make',
                dest='make_option',
                help="options passed to `make`" )

  p.add_option( '-d', '--debug',
                action='store_true',
                dest='debug_build',
                default=False,
                help="enable debug mode build" )

  p.add_option( '-c', '--checkcom',
                action='store_true',
                dest='check_command',
                default=False,
                help="check command line and exit without building" )

  p.add_option( '-y', '--deploy',
                action='store_true',
                dest='deploy_bins',
                default=False,
                help="deploy built binaries" )

  p.add_option( '-?', '--??',
                action='store_true',
                dest='checkusage',
                default=False,
                help='check usage' )

  p.set_defaults( type_qt       = "qt5macports",
                  type_ruby     = "sys",
                  type_python   = "sys",
                  no_qt_binding = False,
                  make_option   = "-j4",
                  debug_build   = False,
                  check_command = False,
                  deploy_bins   = False,
                  checkusage    = False )

  opt, args = p.parse_args()
  if (opt.checkusage):
    print(Usage)
    quit()

  # Determine Qt type
  candidates = [ i.upper() for i in ['Qt4MacPorts', 'Qt5MacPorts'] ]
  ModuleQt   = ""
  index      = 0
  for item in candidates:
    if opt.type_qt.upper() == item:
      if index == 0:
        ModuleQt = 'Qt4MacPorts'
        break
      elif index == 1:
        ModuleQt = 'Qt5MacPorts'
        break
    else:
      index += 1
  if ModuleQt == "":
    print("")
    print( "!!! Unknown Qt type", file=sys.stderr )
    print(Usage)
    quit()

  # Determine Ruby type
  candidates = [ i.upper() for i in ['nil', 'Sys', 'RubySource'] ]
  ModuleRuby = ""
  index      = 0
  for item in candidates:
    if opt.type_ruby.upper() == item:
      if index == 0:
        ModuleRuby = 'nil'
        break
      elif index == 1:
        if Platform == "Yosemite":
          ModuleRuby = 'RubyYosemite'
        elif Platform == "ElCapitan":
          ModuleRuby = 'RubyElCapitan'
        elif Platform == "Sierra":
          ModuleRuby = 'RubySierra'
        elif Platform == "HighSierra":
          ModuleRuby = 'RubyHighSierra'
        else:
          ModuleRuby = ''
        break
      elif index == 2:
        ModuleRuby = 'Ruby24SrcBuild'
    else:
      index += 1
  if ModuleRuby == "":
    print("")
    print( "!!! Unknown Ruby type", file=sys.stderr )
    print(Usage)
    quit()

  # Determine Python type
  candidates = [ i.upper() for i in ['nil', 'Sys', 'Anaconda27', 'Anaconda36'] ]
  ModulePython = ""
  index      = 0
  for item in candidates:
    if opt.type_python.upper() == item:
      if index == 0:
        ModulePython = 'nil'
        break
      elif index == 1:
        if Platform == "Yosemite":
          ModulePython = 'PythonYosemite'
        elif Platform == "ElCapitan":
          ModulePython = 'PythonElCapitan'
        elif Platform == "Sierra":
          ModulePython = 'PythonSierra'
        elif Platform == "HighSierra":
          ModulePython = 'PythonHighSierra'
        else:
          ModulePython = ''
        break
      elif index == 2:
        ModulePython = 'Anaconda27'
      elif index == 3:
        ModulePython = 'Anaconda36'
    else:
      index += 1
  if ModulePython == "":
    print("")
    print( "!!! Unknown Python type", file=sys.stderr )
    print(Usage)
    quit()

  NoQtBindings = opt.no_qt_binding
  MakeOptions  = opt.make_option
  DebugMode    = opt.debug_build
  CheckComOnly = opt.check_command
  Deployment   = opt.deploy_bins

  if not Deployment:
    target  = "%s %s %s" % (Platform, Release, Machine)
    modules = "Qt=%s, Ruby=%s, Python=%s" % (ModuleQt, ModuleRuby, ModulePython)
    message = "### You are going to build KLayout\n    for  <%s>\n    with <%s>...\n"
    print("")
    print( message % (target, modules) )

#------------------------------------------------------------------------------
## To run the main Bash script "build.sh" with appropriate options
#
# @return 0 on success; non-zero on failure
#------------------------------------------------------------------------------
def RunMainBuildBash():
  global ProjectDir
  global Platform
  global BuildBash
  global ModuleQt
  global ModuleRuby
  global ModulePython
  global NoQtBindings
  global MakeOptions
  global DebugMode
  global CheckComOnly
  global Deployment
  global MacPkgDir       # relative path to package directory
  global MacBinDir       # relative path to binary directory
  global MacBuildDir     # relative path to build directory
  global MacBuildLog     # relative path to build log file
  global AbsMacPkgDir    # absolute path to package directory
  global AbsMacBinDir    # absolute path to binary directory
  global AbsMacBuildDir  # absolute path to build directory
  global AbsMacBuildLog  # absolute path to build log file

  #-----------------------------------------------------
  # [1] Set parameters passed to the main Bash script
  #-----------------------------------------------------
  parameters = ""

  # (A) debug or release
  if DebugMode:
    mode        = "debug"
    parameters += "  -debug"
  else:
    mode        = "release"
    parameters += "  -release"

  # (B) Qt4 or Qt5
  if ModuleQt == 'Qt4MacPorts':
    parameters    += " \\\n  -qt4"
    parameters    += " \\\n  -qmake  %s" % Qt4MacPorts['qmake']
    MacPkgDir      = "./qt4.pkg.macos-%s-%s"        % (Platform, mode)
    MacBinDir      = "./qt4.bin.macos-%s-%s"        % (Platform, mode)
    MacBuildDir    = "./qt4.build.macos-%s-%s"      % (Platform, mode)
    MacBuildLog    = "./qt4.build.macos-%s-%s.log"  % (Platform, mode)
    AbsMacPkgDir   = "%s/qt4.pkg.macos-%s-%s"       % (ProjectDir, Platform, mode)
    AbsMacBinDir   = "%s/qt4.bin.macos-%s-%s"       % (ProjectDir, Platform, mode)
    AbsMacBuildDir = "%s/qt4.build.macos-%s-%s"     % (ProjectDir, Platform, mode)
    AbsMacBuildLog = "%s/qt4.build.macos-%s-%s.log" % (ProjectDir, Platform, mode)
    parameters    += " \\\n  -bin    %s" % MacBinDir
    parameters    += " \\\n  -build  %s" % MacBuildDir
  elif ModuleQt == 'Qt5MacPorts':
    parameters    += " \\\n  -qt5"
    parameters    += " \\\n  -qmake  %s" % Qt5MacPorts['qmake']
    MacPkgDir      = "./qt5.pkg.macos-%s-%s"        % (Platform, mode)
    MacBinDir      = "./qt5.bin.macos-%s-%s"        % (Platform, mode)
    MacBuildDir    = "./qt5.build.macos-%s-%s"      % (Platform, mode)
    MacBuildLog    = "./qt5.build.macos-%s-%s.log"  % (Platform, mode)
    AbsMacPkgDir   = "%s/qt5.pkg.macos-%s-%s"       % (ProjectDir, Platform, mode)
    AbsMacBinDir   = "%s/qt5.bin.macos-%s-%s"       % (ProjectDir, Platform, mode)
    AbsMacBuildDir = "%s/qt5.build.macos-%s-%s"     % (ProjectDir, Platform, mode)
    AbsMacBuildLog = "%s/qt5.build.macos-%s-%s.log" % (ProjectDir, Platform, mode)
    parameters    += " \\\n  -bin    %s" % MacBinDir
    parameters    += " \\\n  -build  %s" % MacBuildDir

  # (C) want Qt bindings with Ruby scripts?
  if NoQtBindings:
    parameters += "\\\n  -without-qtbinding"
  else:
    parameters += "\\\n  -with-qtbinding"

  # (D) options to `make` tool
  if not MakeOptions == "":
    parameters += " \\\n  -option %s" % MakeOptions

  # (E) about Ruby
  if ModuleRuby == "nil":
    parameters += " \\\n  -noruby"
  else:
    parameters += " \\\n  -ruby   %s" % RubyDictionary[ModuleRuby]['exe']
    parameters += " \\\n  -rbinc  %s" % RubyDictionary[ModuleRuby]['inc']
    parameters += " \\\n  -rblib  %s" % RubyDictionary[ModuleRuby]['lib']

  # (F) about Python
  if ModulePython == "nil":
    parameters += " \\\n  -nopython"
  else:
    parameters += " \\\n  -python %s" % PythonDictionary[ModulePython]['exe']
    parameters += " \\\n  -pyinc  %s" % PythonDictionary[ModulePython]['inc']
    parameters += " \\\n  -pylib  %s" % PythonDictionary[ModulePython]['lib']

  #-----------------------------------------------------
  # [2] Make the consolidated command line
  #-----------------------------------------------------
  command  = "time"
  command += " \\\n  %s" % BuildBash
  command += parameters
  command += "  2>&1 | tee %s" % MacBuildLog
  if CheckComOnly:
    print(command)
    quit()

  #-----------------------------------------------------
  # [3] Invoke the main Bash script
  #-----------------------------------------------------
  if Deployment:
    return(0)
  else:
    myscript = "build4mac.py"
    if subprocess.call( command, shell=True ) != 0:
      print("")
      print( "-------------------------------------------------------------" )
      print( "!!! <%s>: failed to build KLayout" % myscript, file=sys.stderr )
      print( "-------------------------------------------------------------" )
      print("")
      return(1)
    else:
      print("")
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" )
      print( "### <%s>: successfully built KLayout" % myscript, file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" )
      print("")
      return(0)

#------------------------------------------------------------------------------
## To deploy built binaries and depending libraries for making a bundle
#
# Reference: "Deploying an Application on Mac OS X" of Qt Assistant.
#
# @return 0 on success; non-zero on failure
#------------------------------------------------------------------------------
def DeployBinariesForBundle():
  global ProjectDir
  global ModuleQt
  global Deployment
  global MacPkgDir
  global MacBinDir
  global MacBuildDir
  global MacBuildLog
  global AbsMacPkgDir
  global AbsMacBinDir
  global AbsMacBuildDir
  global AbsMacBuildLog

  print("")
  print( "##### Start deploying libraries and executables #####" )
  print( " [1] Checking the status of working directory ..." )
  #-------------------------------------------------------------
  # [1] Check the status of working directory
  #-------------------------------------------------------------
  os.chdir(ProjectDir)
  if not Deployment:
    return(1)
  if not os.path.isfile(MacBuildLog):
    print( "!!! Build log file <%s> does not present !!!" % MacBuildLog, file=sys.stderr )
    return(1)
  if not os.path.isdir(MacBuildDir):
    print( "!!! Build directory <%s> does not present !!!" % MacBuildDir, file=sys.stderr )
    return(1)
  if not os.path.isdir(MacBinDir):
    print( "!!! Binary directory <%s> does not present !!!" % MacBinDir, file=sys.stderr )
    return(1)


  print( " [2] Creating a new empty directory <%s> for deployment ..." % MacPkgDir )
  #-------------------------------------------------------------
  # [2] Create a new empty directory for deploying binaries
  #-------------------------------------------------------------
  os.chdir(ProjectDir)
  if os.path.isfile(MacPkgDir):
    os.remove(MacPkgDir)
  if os.path.isdir(MacPkgDir):
    shutil.rmtree(MacPkgDir)
  os.mkdir(MacPkgDir)


  print( " [3] Creating the standard directory structure for a bundle ..." )
  #-------------------------------------------------------------
  # [3] Create the directory skeleton for "klayout.app" bundle
  #     and command line buddy tools such as "strm2cif".
  #     They should be stored in a directory structure like...
  #
  #    klayout.app/+
  #                +-- Contents/+
  #                             +-- Info.plist
  #                             +-- PkgInfo
  #                             +-- Resources/
  #                             +-- Frameworks/
  #                             +-- MacOS/+
  #                             |         +-- 'klayout'
  #                             +-- Buddy/+
  #                                       +-- 'strm2cif'
  #                                       +-- 'strm2dxf'
  #                                       :
  #                                       +-- 'strmxor'
  #-------------------------------------------------------------
  targetDir0 = "%s/klayout.app/Contents" % AbsMacPkgDir
  targetDirR = targetDir0 + "/Resources"
  targetDirF = targetDir0 + "/Frameworks"
  targetDirM = targetDir0 + "/MacOS"
  targetDirB = targetDir0 + "/Buddy"
  os.makedirs(targetDirR)
  os.makedirs(targetDirF)
  os.makedirs(targetDirM)
  os.makedirs(targetDirB)


  print( " [4] Copying KLayout's dynamic link libraries to 'Frameworks' ..." )
  #-------------------------------------------------------------------------------
  # [4] Copy KLayout's dynamic link libraries to "Frameworks/" and create
  #     the library dependency dictionary.
  #     <<< Do this job in "Frameworks/" >>>
  #
  # Note:
  #     KLayout's dynamic link library is built as below:
  #       (1) libklayout_lay.0.25.0.dylib
  #       (2) libklayout_lay.0.25.dylib -> libklayout_lay.0.25.0.dylib
  #       (3) libklayout_lay.0.dylib -> libklayout_lay.0.25.0.dylib
  #       (4) libklayout_lay.dylib -> libklayout_lay.0.25.0.dylib
  #     where,
  #       (1) is an ordinary file with full version number 'major.minor.teeny'
  #           between "library_name."='libklayout_lay.' and '.dylib'
  #       (2) is a symbolic link with 'major.minor' version number
  #       (3) is a symbolic link with 'major' version number
  #       (4) is a symbolic link without version number number
  #
  #     The dynamic linker tries to find a library name in style (3) as shown
  #     in the example below.
  #
  # Example:
  #   MacBookPro(1)$ otool -L klayout
  #   klayout:
  #       :
  #       :
  #     libklayout_tl.0.dylib (compatibility version 0.25.0, current version 0.25.0)
  #     libklayout_gsi.0.dylib (compatibility version 0.25.0, current version 0.25.0)
  #     libklayout_db.0.dylib (compatibility version 0.25.0, current version 0.25.0)
  #       :
  #-------------------------------------------------------------------------------
  os.chdir( targetDirF )
  dynamicLinkLibs = glob.glob( AbsMacBinDir + "/*.dylib" )
  depDicOrdinary  = {} # inter-library dependency dictionary
  for item in dynamicLinkLibs:
    if os.path.isfile(item) and not os.path.islink(item):
      #-------------------------------------------------------------------
      # (A) Copy an ordinary *.dylib file here by changing the name
      #     to style (3) and set its mode to 0755 (sanity check).
      #-------------------------------------------------------------------
      fullName = os.path.basename(item).split('.')
      # e.g. [ 'libklayout_lay', '0', '25', '0', 'dylib' ]
      nameStyle3 = fullName[0] + "." + fullName[1] + ".dylib"
      shutil.copy2( item, nameStyle3 )
      os.chmod( nameStyle3, 0755 )
      #-------------------------------------------------------------------
      # (B) Then get inter-library dependencies
      #-------------------------------------------------------------------
      otoolCm   = "otool -L %s | grep libklayout" % nameStyle3
      otoolOut  = os.popen( otoolCm ).read()
      dependDic = DecomposeLibraryDependency(otoolOut)
      depDicOrdinary.update(dependDic)
  '''
  PrintLibraryDependencyDictionary( depDicOrdinary, "Style (3)" )
  quit()
  '''

  print( " [5] Setting and changing the identification names among KLayout's libraries ..." )
  #-------------------------------------------------------------
  # [5] Set the identification names for KLayout's libraries
  #     and make the library aware of the locations of libraries
  #     on which it depends; that is, inter-library dependency
  #-------------------------------------------------------------
  ret = SetChangeIdentificationNameOfDyLib( depDicOrdinary )
  if not ret == 0:
    msg = "!!! Failed to set and change to new identification names !!!"
    print(msg)
    return(1)


  print( " [6] Copying built executables and resource files ..." )
  #-------------------------------------------------------------
  # [6] Copy some known files in source directories to
  #     relevant target directories
  #-------------------------------------------------------------
  os.chdir(ProjectDir)
  sourceDir0 = "%s/klayout.app/Contents" % MacBinDir
  sourceDir1 = sourceDir0 + "/MacOS"
  sourceDir2 = "%s/etc" % ProjectDir
  sourceDir3 = "%s/src/klayout_main" % ProjectDir
  sourceDir4 = "%s" % MacBinDir

  shutil.copy2( sourceDir0 + "/Info.plist", targetDir0 )
  shutil.copy2( sourceDir1 + "/klayout",    targetDirM )
  shutil.copy2( sourceDir2 + "/logo.png",   targetDirR )
  shutil.copy2( sourceDir3 + "/logo.ico",   targetDirR )

  os.chmod( targetDir0 + "/Info.plist", 0644 )
  os.chmod( targetDirM + "/klayout",    0755 )
  os.chmod( targetDirR + "/logo.png",   0644 )
  os.chmod( targetDirR + "/logo.ico",   0644 )

  buddies = glob.glob( sourceDir4 + "/strm*" )
  for item in buddies:
    shutil.copy2( item, targetDirB )
    buddy = os.path.basename(item)
    os.chmod( targetDirB + "/" + buddy, 0755 )


  print( " [7] Setting and changing the identification names of KLayout's libraries in each executable ..." )
  #-------------------------------------------------------------
  # [7] Set and change the library identification name(s) of
  #     different executables
  #-------------------------------------------------------------
  os.chdir(ProjectDir)
  os.chdir(MacPkgDir)
  klayoutexec = "klayout.app/Contents/MacOS/klayout"
  ret = SetChangeLibIdentificationName( klayoutexec, "../Frameworks" )
  if not ret == 0:
    os.chdir(ProjectDir)
    msg = "!!! Failed to set/change library identification name for <%s> !!!"
    print( msg % klayoutexec, file=sys.stderr )
    return(1)

  buddies     = glob.glob( "klayout.app/Contents/Buddy/strm*" )
  macdepQtOpt = ""
  for buddy in buddies:
    macdepQtOpt += "-executable=%s " % buddy
    ret = SetChangeLibIdentificationName( buddy, "../Frameworks" )
    if not ret == 0:
      os.chdir(ProjectDir)
      msg = "!!! Failed to set/change library identification name for <%s> !!!"
      print( msg % buddy, file=sys.stderr )
      return(1)


  print( " [8] Finally, deploying Qt's frameworks ..." )
  #-------------------------------------------------------------
  # [8] Deploy Qt frameworks
  #-------------------------------------------------------------
  if ModuleQt == 'Qt4MacPorts':
    deploytool = Qt4MacPorts['deploy']
    app_bundle = "klayout.app"
    options    = macdepQtOpt
  elif ModuleQt == 'Qt5MacPorts':
    deploytool = Qt5MacPorts['deploy']
    app_bundle = "klayout.app"
    options    = macdepQtOpt

  os.chdir(ProjectDir)
  os.chdir(MacPkgDir)
  command = "%s %s %s" % ( deploytool, app_bundle, options )
  if subprocess.call( command, shell=True ) != 0:
    msg = "!!! Failed to deploy applications on OSX !!!"
    print( msg, file=sys.stderr )
    print("")
    os.chdir(ProjectDir)
    return(1)
  else:
    msg = "### Deployed applications on OS X ###"
    print( msg, file=sys.stderr )
    print("")
    os.chdir(ProjectDir)
    return(0)

#------------------------------------------------------------------------------
## The main function
#------------------------------------------------------------------------------
def main():
  SetGlobals()
  ParseCommandLineArguments()
  ret = RunMainBuildBash()
  if not Deployment:
    if ret == 0:
      sys.exit(0)
    else:
      sys.exit(1)
  else:
    ret = DeployBinariesForBundle()
    sys.exit(ret)

#===================================================================================
if __name__ == "__main__":
  main()

#---------------
# End of file
#---------------
