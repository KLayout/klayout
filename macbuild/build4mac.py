#!/usr/bin/env python
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
## To import global dictionaries of different modules and utility functions
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
  global NonOSStdLang       # True if non-OS-standard language is chosen
  global NoQtBindings       # True if not creating Qt bindings for Ruby scripts
  global MakeOptions        # options passed to `make`
  global DebugMode          # True if debug mode build
  global CheckComOnly       # True if only for checking the command line parameters to "build.sh"
  global DeploymentF        # True if fully (including Qt's Frameworks) deploy the binaries for bundles
  global DeploymentP        # True if partially deploy the binaries excluding Qt's Frameworks
  global DeployVerbose      # -verbose=<0-3> level passed to 'macdeployqt' tool
  global Version            # KLayout's version
  # auxiliary variables on platform
  global System             # 6-tuple from platform.uname()
  global Node               # - do -
  global Release            # - do -
  global Version            # - do -
  global Machine            # - do -
  global Processor          # - do -
  global Bit                # machine bit-size

  Usage  = "\n"
  Usage += "---------------------------------------------------------------------------------------------------------\n"
  Usage += "<< Usage of 'build4mac.py' >>\n"
  Usage += "       for building KLayout 0.25 or later on different Apple Mac OSX platforms.\n"
  Usage += "\n"
  Usage += "$ [python] ./build4mac.py \n"
  Usage += "   option & argument    : descriptions                                                   | default value\n"
  Usage += "   --------------------------------------------------------------------------------------+---------------\n"
  Usage += "                        : * key type names below are case insensitive *                  | \n"
  Usage += "                        :   'nil' = not to support the script language                   | \n"
  Usage += "                        :   'Sys' = using the OS standard script language                | \n"
  Usage += "                        : Refer to 'macbuild/build4mac_env.py' for details               | \n"
  Usage += "   [-q|--qt <type>]     : type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3']                     | qt5macports \n"
  Usage += "   [-r|--ruby <type>]   : type=['nil', 'Sys', 'MP24', 'B25', 'Ana3']                     | sys \n"
  Usage += "   [-p|--python <type>] : type=['nil', 'Sys', 'MP36', 'B37', 'Ana3']                     | sys \n"
  Usage += "   [-n|--noqtbinding]   : don't create Qt bindings for ruby scripts                      | disabled \n"
  Usage += "   [-m|--make <option>] : option passed to 'make'                                        | -j4 \n"
  Usage += "   [-d|--debug]         : enable debug mode build                                        | disabled \n"
  Usage += "   [-c|--checkcom]      : check command line and exit without building                   | disabled \n"
  Usage += "   [-y|--deploy]        : deploy executables and dylibs including Qt's Frameworks        | disabled \n"
  Usage += "   [-Y|--DEPLOY]        : deploy executables and dylibs for those who built KLayout      | disabled \n"
  Usage += "                        : from the source code and use the tools in the same machine     | \n"
  Usage += "                        : ! After confirmation of successful build of                    | \n"
  Usage += "                        :   KLayout, rerun this script with BOTH:                        | \n"
  Usage += "                        :     1) the same options used for building AND                  | \n"
  Usage += "                        :     2) <-y|--deploy> OR <-Y|--DEPLOY>                          | \n"
  Usage += "                        :   optionally with [-v|--verbose <0-3>]                         | \n"
  Usage += "   [-v|--verbose <0-3>] : verbose level of `macdeployqt' (effective with -y only)        | 1 \n"
  Usage += "                        : 0 = no output, 1 = error/warning (default),                    | \n"
  Usage += "                        : 2 = normal,    3 = debug                                       | \n"
  Usage += "   [-?|--?]             : print this usage and exit                                      | disabled \n"
  Usage += "---------------------------------------------------------------------------------------------------------\n"

  ProjectDir = os.getcwd()
  BuildBash  = "./build.sh"
  (System, Node, Release, Version, Machine, Processor) = platform.uname()

  if not System == "Darwin":
    print("")
    print( "!!! Sorry. Your system <%s> looks like non-Mac" % System, file=sys.stderr )
    print(Usage)
    sys.exit(1)

  release = int( Release.split(".")[0] ) # take the first of ['14', '5', '0']
  if release == 14:
    Platform = "Yosemite"
  elif release == 15:
    Platform = "ElCapitan"
  elif release == 16:
    Platform = "Sierra"
  elif release == 17:
    Platform = "HighSierra"
  elif release == 18:
    Platform = "Mojave"
  else:
    Platform = ""
    print("")
    print( "!!! Sorry. Unsupported major OS release <%d>" % release, file=sys.stderr )
    print(Usage)
    sys.exit(1)

  if not Machine == "x86_64":
    print("")
    print( "!!! Sorry. Only x86_64 architecture machine is supported but found <%s>" % Machine, file=sys.stderr )
    print(Usage)
    sys.exit(1)

  # default modules
  ModuleQt = "Qt5MacPorts"
  if Platform == "Yosemite":
    ModuleRuby   = "RubyYosemite"
    ModulePython = "PythonYosemite"
  elif Platform == "ElCapitan":
    ModuleRuby   = "RubyElCapitan"
    ModulePython = "PythonElCapitan"
  elif Platform == "Sierra":
    ModuleRuby   = "RubySierra"
    ModulePython = "PythonSierra"
  elif Platform == "HighSierra":
    ModuleRuby   = "RubyHighSierra"
    ModulePython = "PythonHighSierra"
  elif Platform == "Mojave":
    ModuleRuby   = "RubyMojave"
    ModulePython = "PythonMojave"
  else:
    ModuleRuby   = "nil"
    ModulePython = "nil"

  NonOSStdLang  = False
  NoQtBindings  = False
  MakeOptions   = "-j4"
  DebugMode     = False
  CheckComOnly  = False
  DeploymentF   = False
  DeploymentP   = False
  DeployVerbose = 1
  Version       = GetKLayoutVersionFrom( "./version.sh" )

#------------------------------------------------------------------------------
## To get command line parameters
#------------------------------------------------------------------------------
def ParseCommandLineArguments():
  global Usage
  global Platform
  global ModuleQt
  global ModuleRuby
  global ModulePython
  global NonOSStdLang
  global NoQtBindings
  global MakeOptions
  global DebugMode
  global CheckComOnly
  global DeploymentF
  global DeploymentP
  global DeployVerbose

  p = optparse.OptionParser( usage=Usage )
  p.add_option( '-q', '--qt',
                dest='type_qt',
                help="Qt type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3']" )

  p.add_option( '-r', '--ruby',
                dest='type_ruby',
                help="Ruby type=['nil', 'Sys', 'MP24', 'B25', 'Ana3']" )

  p.add_option( '-p', '--python',
                dest='type_python',
                help="Python type=['nil', 'Sys', 'MP36', 'B37', 'Ana3']" )

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
                dest='deploy_full',
                default=False,
                help="fully deploy built binaries" )

  p.add_option( '-Y', '--DEPLOY',
                action='store_true',
                dest='deploy_partial',
                default=False,
                help="deploy built binaries when non-OS-standard script language is chosen" )

  p.add_option( '-v', '--verbose',
                dest='deploy_verbose',
                help="verbose level of `macdeployqt` tool" )

  p.add_option( '-?', '--??',
                action='store_true',
                dest='checkusage',
                default=False,
                help='check usage' )

  p.set_defaults( type_qt        = "qt5macports",
                  type_ruby      = "sys",
                  type_python    = "sys",
                  no_qt_binding  = False,
                  make_option    = "-j4",
                  debug_build    = False,
                  check_command  = False,
                  deploy_full    = False,
                  deploy_partial = False,
                  deploy_verbose = "1",
                  checkusage     = False )

  opt, args = p.parse_args()
  if (opt.checkusage):
    print(Usage)
    sys.exit(0)

  # Determine Qt type
  candidates = ['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3']
  candidates_upper = [ i.upper() for i in candidates ]
  ModuleQt = ""
  index    = 0
  if opt.type_qt.upper() in candidates_upper:
    idx = candidates_upper.index(opt.type_qt.upper())
    ModuleQt = candidates[idx]
  else:
    print("")
    print( "!!! Unknown Qt type %s. Candidates: %s" % (opt.type_qt, candidates), file=sys.stderr )
    print(Usage)
    sys.exit(1)

  # By default, OS-standard script languages (Ruby and Python) are used
  NonOSStdLang = False

  # Determine Ruby type
  candidates = [ i.upper() for i in ['nil', 'Sys', 'MP24', 'B25', 'Ana3'] ]
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
        elif Platform == "Mojave":
          ModuleRuby = 'RubyMojave'
        else:
          ModuleRuby = ''
        break
      elif index == 2:
        ModuleRuby   = 'Ruby24MacPorts'
        NonOSStdLang = True
      elif index == 3:
        ModuleRuby   = 'Ruby25Brew'
        NonOSStdLang = True
      elif index == 4:
        ModuleRuby   = 'RubyAnaconda3'
        NonOSStdLang = True
    else:
      index += 1
  if ModuleRuby == "":
    print("")
    print( "!!! Unknown Ruby type", file=sys.stderr )
    print(Usage)
    sys.exit(1)

  # Determine Python type
  candidates   = [ i.upper() for i in ['nil', 'Sys', 'MP36', 'B37', 'Ana3'] ]
  ModulePython = ""
  index        = 0
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
        elif Platform == "Mojave":
          ModulePython = 'PythonMojave'
        else:
          ModulePython = ''
        break
      elif index == 2:
        ModulePython = 'Python36MacPorts'
        NonOSStdLang = True
      elif index == 3:
        ModulePython = 'Python37Brew'
        NonOSStdLang = True
      elif index == 4:
        ModulePython = 'PythonAnaconda3'
        NonOSStdLang = True
    else:
      index += 1
  if ModulePython == "":
    print("")
    print( "!!! Unknown Python type", file=sys.stderr )
    print(Usage)
    sys.exit(1)

  NoQtBindings  = opt.no_qt_binding
  MakeOptions   = opt.make_option
  DebugMode     = opt.debug_build
  CheckComOnly  = opt.check_command
  DeploymentF   = opt.deploy_full
  DeploymentP   = opt.deploy_partial

  if DeploymentF and DeploymentP:
    print("")
    print( "!!! Choose either [-y|--deploy] or [-Y|--DEPLOY]", file=sys.stderr )
    print(Usage)
    sys.exit(1)

  DeployVerbose = int(opt.deploy_verbose)
  if not DeployVerbose in [0, 1, 2, 3]:
    print("")
    print( "!!! Unsupported verbose level passed to `macdeployqt` tool", file=sys.stderr )
    print(Usage)
    sys.exit(1)

  if not DeploymentF and not DeploymentP:
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
  global DeploymentF
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

  # (B) Qt5
  if ModuleQt == 'Qt5MacPorts':
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
  elif ModuleQt == 'Qt5Brew':
    parameters    += " \\\n  -qt5"
    parameters    += " \\\n  -qmake  %s" % Qt5Brew['qmake']
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
  elif ModuleQt == 'Qt5Ana3':
    parameters    += " \\\n  -qt5"
    parameters    += " \\\n  -qmake  %s" % Qt5Ana3['qmake']
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
  parameters += " \\\n  -rpath    %s" % "@executable_path/../Frameworks"

  # (C) want Qt bindings with Ruby scripts?
  if NoQtBindings:
    parameters += " \\\n  -without-qtbinding"
  else:
    parameters += " \\\n  -with-qtbinding"

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
    sys.exit(0)

  #-----------------------------------------------------
  # [3] Invoke the main Bash script; takes time:-)
  #-----------------------------------------------------
  if DeploymentF:
    return 0
  elif DeploymentP:
    return 0
  else:
    myscript = "build4mac.py"
    if subprocess.call( command, shell=True ) != 0:
      print( "", file=sys.stderr )
      print( "-------------------------------------------------------------", file=sys.stderr )
      print( "!!! <%s>: failed to build KLayout" % myscript, file=sys.stderr )
      print( "-------------------------------------------------------------", file=sys.stderr )
      print( "", file=sys.stderr )
      return 1
    else:
      print( "", file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "### <%s>: successfully built KLayout" % myscript, file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "", file=sys.stderr )
      return 0

#------------------------------------------------------------------------------
## For making a bundle (klayout.app), deploy built binaries and libraries
#  on which those binaries depend.
#
# Reference: "Deploying an Application on Mac OS X" of Qt Assistant.
#
# @return 0 on success; non-zero on failure
#------------------------------------------------------------------------------
def DeployBinariesForBundle():
  global ProjectDir
  global ModuleQt
  global NonOSStdLang
  global DeploymentF
  global DeploymentP
  global MacPkgDir
  global MacBinDir
  global MacBuildDir
  global MacBuildLog
  global AbsMacPkgDir
  global AbsMacBinDir
  global AbsMacBuildDir
  global AbsMacBuildLog
  global Version
  global DeployVerbose

  print("")
  print( "##### Started deploying libraries and executables for <klayout.app> #####" )
  print( " [1] Checking the status of working directory ..." )
  #-------------------------------------------------------------
  # [1] Check the status of working directory
  #-------------------------------------------------------------
  os.chdir(ProjectDir)
  if not DeploymentF and not DeploymentP:
    return 1
  if DeploymentF and NonOSStdLang:
    print( "     WARNING!!! You chose <-y|--deploy> while using non-OS-standard script language.", file=sys.stderr )
    print( "         Consider using <-Y|--DEPLOY> instead", file=sys.stderr )
    #return 1
  if not os.path.isfile(MacBuildLog):
    print( "!!! Build log file <%s> does not present !!!" % MacBuildLog, file=sys.stderr )
    return 1
  if not os.path.isdir(MacBuildDir):
    print( "!!! Build directory <%s> does not present !!!" % MacBuildDir, file=sys.stderr )
    return 1
  if not os.path.isdir(MacBinDir):
    print( "!!! Binary directory <%s> does not present !!!" % MacBinDir, file=sys.stderr )
    return 1


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


  print( " [3] Creating the standard directory structure for 'klayout.app' bundle ..." )
  #-----------------------------------------------------------------
  # [3] Create the directory skeleton for "klayout.app" bundle
  #     and command line buddy tools such as "strm2cif".
  #     They are stored in the directory structure below.
  #
  #    klayout.app/+
  #                +-- Contents/+
  #                             +-- Info.plist
  #                             +-- PkgInfo
  #                             +-- Resources/+
  #                             |             +-- 'klayout.icns'
  #                             +-- Frameworks/+
  #                             |              +-- '*.framework'
  #                             |              +-- '*.dylib'
  #                             +-- MacOS/+
  #                             |         +-- 'klayout'
  #                             +-- Buddy/+
  #                                       +-- 'strm2cif'
  #                                       +-- 'strm2dxf'
  #                                       :
  #                                       +-- 'strmxor'
  #-----------------------------------------------------------------
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
      os.chmod( nameStyle3, 0o0755 )
      #-------------------------------------------------------------------
      # (B) Then get inter-library dependencies
      #-------------------------------------------------------------------
      otoolCm   = "otool -L %s | grep libklayout" % nameStyle3
      otoolOut  = os.popen( otoolCm ).read()
      dependDic = DecomposeLibraryDependency(otoolOut)
      depDicOrdinary.update(dependDic)
  '''
  PrintLibraryDependencyDictionary( depDicOrdinary, "Style (3)" )
  sys.exit(0)
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
    return 1


  print( " [6] Copying built executables and resource files ..." )
  #-------------------------------------------------------------
  # [6] Copy some known files in source directories to
  #     relevant target directories
  #-------------------------------------------------------------
  os.chdir(ProjectDir)
  sourceDir0 = "%s/klayout.app/Contents" % MacBinDir
  sourceDir1 = sourceDir0 + "/MacOS"
  sourceDir2 = "%s/macbuild/Resources" % ProjectDir
  sourceDir3 = "%s" % MacBinDir

  tmpfileM = ProjectDir + "/macbuild/Resources/Info.plist.template"
  keydicM  = { 'exe': 'klayout', 'icon': 'klayout.icns', 'bname': 'klayout', 'ver': Version }
  plistM   = GenerateInfoPlist( keydicM, tmpfileM )
  file     = open( targetDir0 + "/Info.plist", "w" )
  file.write(plistM)
  file.close()

  shutil.copy2( sourceDir0 + "/PkgInfo",      targetDir0 ) # this file is not mandatory
  shutil.copy2( sourceDir1 + "/klayout",      targetDirM )
  shutil.copy2( sourceDir2 + "/klayout.icns", targetDirR )


  os.chmod( targetDir0 + "/PkgInfo",      0o0644 )
  os.chmod( targetDir0 + "/Info.plist",   0o0644 )
  os.chmod( targetDirM + "/klayout",      0o0755 )
  os.chmod( targetDirR + "/klayout.icns", 0o0644 )

  buddies = glob.glob( sourceDir3 + "/strm*" )
  for item in buddies:
    shutil.copy2( item, targetDirB )
    buddy = os.path.basename(item)
    os.chmod( targetDirB + "/" + buddy, 0o0755 )


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
    return 1

  buddies     = glob.glob( "klayout.app/Contents/Buddy/strm*" )
  macdepQtOpt = ""
  for buddy in buddies:
    macdepQtOpt += " -executable=%s" % buddy
    ret = SetChangeLibIdentificationName( buddy, "../Frameworks" )
    if not ret == 0:
      os.chdir(ProjectDir)
      msg = "!!! Failed to set/change library identification name for <%s> !!!"
      print( msg % buddy, file=sys.stderr )
      return 1

  if DeploymentF:
    print( " [8] Finally, deploying Qt's Frameworks ..." )
    #-------------------------------------------------------------
    # [8] Deploy Qt Frameworks
    #-------------------------------------------------------------
    verbose = " -verbose=%d" % DeployVerbose
    if ModuleQt == 'Qt5MacPorts':
      deploytool = Qt5MacPorts['deploy']
      app_bundle = "klayout.app"
      options    = macdepQtOpt + verbose
    elif ModuleQt == 'Qt5Brew':
      deploytool = Qt5Brew['deploy']
      app_bundle = "klayout.app"
      options    = macdepQtOpt + verbose
    elif ModuleQt == 'Qt5Ana3':
      deploytool = Qt5Ana3['deploy']
      app_bundle = "klayout.app"
      options    = macdepQtOpt + verbose

    # Without the following, the plugin cocoa would not be found properly.
    shutil.copy2( sourceDir2 + "/qt.conf", targetDirM )
    os.chmod( targetDirM + "/qt.conf",      0o0644 )

    os.chdir(ProjectDir)
    os.chdir(MacPkgDir)
    command = "%s %s %s" % ( deploytool, app_bundle, options )
    if subprocess.call( command, shell=True ) != 0:
      msg = "!!! Failed to deploy applications on OSX !!!"
      print( msg, file=sys.stderr )
      print("")
      os.chdir(ProjectDir)
      return 1

    deploymentPython = (ModulePython == 'Python37Brew')
    #deploymentPython = True
    if deploymentPython and NonOSStdLang:
      from build4mac_util import WalkFrameworkPaths, PerformChanges

      bundlePath = AbsMacPkgDir + '/klayout.app'
      # bundlePath = os.getcwd() + '/qt5.pkg.macos-HighSierra-release/klayout.app'
      bundleExecPathAbs = '%s/Contents/MacOS/' % bundlePath
      pythonOriginalFrameworkPath = '/usr/local/opt/python/Frameworks/Python.framework'
      pythonFrameworkPath = '%s/Contents/Frameworks/Python.framework' % bundlePath

      print(" [8.1] Deploying Python from %s ..." % pythonOriginalFrameworkPath)
      print("  [1] Copying Python Framework")
      shell_commands = list()
      shell_commands.append("rm -rf %s" % pythonFrameworkPath)
      shell_commands.append("rsync -a --safe-links %s/ %s" % (pythonOriginalFrameworkPath, pythonFrameworkPath))
      shell_commands.append("mkdir %s/Versions/3.7/lib/python3.7/site-packages/" % pythonFrameworkPath)
      shell_commands.append("cp -RL %s/Versions/3.7/lib/python3.7/site-packages/{pip*,pkg_resources,setuptools*,wheel*} " % pythonOriginalFrameworkPath +
                            "%s/Versions/3.7/lib/python3.7/site-packages/" % pythonFrameworkPath)
      shell_commands.append("rm -rf %s/Versions/3.7/lib/python3.7/test" % pythonFrameworkPath)
      shell_commands.append("rm -rf %s/Versions/3.7/Resources" % pythonFrameworkPath)
      shell_commands.append("rm -rf %s/Versions/3.7/bin" % pythonFrameworkPath)

      for command in shell_commands:
        if subprocess.call( command, shell=True ) != 0:
          msg = "command failed: %s"
          print( msg % command, file=sys.stderr )
          sys.exit(1)

      shutil.copy2( sourceDir2 + "/start-console.py", targetDirM )
      shutil.copy2( sourceDir2 + "/klayout_console", targetDirM )
      os.chmod( targetDirM + "/klayout_console",      0o0755 )

      print("  [2] Relinking dylib dependencies inside Python.framework")
      print("   [2.1] Patching Python Framework")
      depdict = WalkFrameworkPaths(pythonFrameworkPath)
      appPythonFrameworkPath = '@executable_path/../Frameworks/Python.framework/'
      PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs)

      print("   [2.2] Patching /usr/local/opt/ libs")
      usrLocalPath = '/usr/local/opt/'
      appUsrLocalPath = '@executable_path/../Frameworks/'
      replacePairs = [(usrLocalPath, appUsrLocalPath, True)]
      depdict = WalkFrameworkPaths(pythonFrameworkPath, search_path_filter=r'\t+/usr/local/(opt|Cellar)')
      PerformChanges(depdict, replacePairs, bundleExecPathAbs)

      print("   [2.3] Patching openssl, gdbm, readline, sqlite, tcl-tk, xz")
      usrLocalPath = '/usr/local/opt'
      appUsrLocalPath = '@executable_path/../Frameworks/'
      replacePairs = [(usrLocalPath, appUsrLocalPath, True)]
      replacePairs.extend([(openssl_version, '@executable_path/../Frameworks/openssl', True)
        for openssl_version in glob.glob('/usr/local/Cellar/openssl/*')])
      depdict = WalkFrameworkPaths([pythonFrameworkPath + '/../openssl',
                                    pythonFrameworkPath + '/../gdbm',
                                    pythonFrameworkPath + '/../readline',
                                    pythonFrameworkPath + '/../sqlite',
                                    pythonFrameworkPath + '/../tcl-tk',
                                    pythonFrameworkPath + '/../xz'], search_path_filter=r'\t+/usr/local/(opt|Cellar)')

      PerformChanges(depdict, replacePairs, bundleExecPathAbs)

      print("  [3] Relinking dylib dependencies for klayout")
      klayoutPath = bundleExecPathAbs
      depdict = WalkFrameworkPaths(klayoutPath, filter_regex=r'klayout$')
      PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs)

      libKlayoutPath = bundleExecPathAbs + '../Frameworks'
      depdict = WalkFrameworkPaths(libKlayoutPath, filter_regex=r'libklayout')
      PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs)

      print("  [4] Patching site.py, pip/, and distutils/")
      site_module = "%s/Versions/3.7/lib/python3.7/site.py" % pythonFrameworkPath
      with open(site_module, 'r') as site:
        buf = site.readlines()
      with open(site_module, 'w') as site:
        import re
        for line in buf:
          # This will fool pip into thinking it's inside a virtual environment
          # and install new packates to the correct site-packages
          if re.match("^PREFIXES", line) is not None:
            line = line + "sys.real_prefix = sys.prefix\n"
          # do not allow installation in the user folder.
          if re.match("^ENABLE_USER_SITE", line) is not None:
            line = "ENABLE_USER_SITE = False\n"
          site.write(line)

      pip_module = "%s/Versions/3.7/lib/python3.7/site-packages/pip/__init__.py" % pythonFrameworkPath
      with open(pip_module, 'r') as pip:
        buf = pip.readlines()
      with open(pip_module, 'w') as pip:
        import re
        for line in buf:
          # this will reject user's configuration of pip, forcing the isolated mode
          line = re.sub("return isolated$", "return isolated or True", line)
          pip.write(line)

      distutilsconfig = "%s/Versions/3.7/lib/python3.7/distutils/distutils.cfg" % pythonFrameworkPath
      with open(distutilsconfig, 'r') as file:
        buf = file.readlines()
      with open(distutilsconfig, 'w') as file:
        import re
        for line in buf:
          # This will cause all packages to be installed to sys.prefix
          if re.match('prefix=', line) is not None:
            continue
          file.write(line)

  else:
    print( " [8] Skipped deploying Qt's Frameworks ..." )
  print( "##### Finished deploying libraries and executables for <klayout.app> #####" )
  print("")
  os.chdir(ProjectDir)
  return 0
#------------------------------------------------------------------------------
## To deploy script bundles that invoke the main bundle (klayout.app) in
#  editor mode or viewer mode
#
# @return 0 on success; non-zero on failure
#------------------------------------------------------------------------------
def DeployScriptBundles():
  global ProjectDir
  global MacPkgDir
  global AbsMacPkgDir

  print("")
  print( "##### Started deploying files for <KLayoutEditor.app> and <KLayoutViewer.app> #####" )
  print( " [1] Checking the status of working directory ..." )
  #-------------------------------------------------------------
  # [1] Check the status of working directory
  #-------------------------------------------------------------
  os.chdir(ProjectDir)
  if not os.path.isdir(MacPkgDir):
    print( "!!! Package directory <%s> does not present !!!" % MacPkgDir, file=sys.stderr )
    return 1


  print( " [2] Creating a new empty directory <%s> for deployment ..." % (MacPkgDir + "/klayout.scripts") )
  #-------------------------------------------------------------
  # [2] Create a new empty directory for deploying binaries
  #-------------------------------------------------------------
  os.chdir(MacPkgDir)
  scriptDir = "klayout.scripts"
  if os.path.isfile(scriptDir):
    os.remove(scriptDir)
  if os.path.isdir(scriptDir):
    shutil.rmtree(scriptDir)
  os.mkdir(scriptDir)


  print( " [3] Creating the standard directory structure for the script bundles ..." )
  #--------------------------------------------------------------------------------------------
  # [3] Create the directory skeleton for the two script bundles.
  #
  #  klayout.scripts/+
  #                  +-- KLayoutEditor.app/+
  #                  |                     +-- Contents/+
  #                  |                                  +-- Info.plist
  #                  |                                  +-- Resources/+
  #                  |                                  |             +-- 'klayout-red.icns'
  #                  |                                  +-- MacOS/+
  #                  |                                            +-- 'KLayoutEditor.sh'
  #                  +-- KLayoutViewer.app/+
  #                                        +-- Contents/+
  #                                                     +-- Info.plist
  #                                                     +-- Resources/+
  #                                                     |             +-- 'klayout-blue.icns'
  #                                                     +-- MacOS/+
  #                                                               +-- 'KLayoutViewer.sh'
  #--------------------------------------------------------------------------------------------
  os.chdir(ProjectDir)
  targetDir0E = "%s/%s/KLayoutEditor.app/Contents" % ( AbsMacPkgDir, scriptDir )
  targetDir0V = "%s/%s/KLayoutViewer.app/Contents" % ( AbsMacPkgDir, scriptDir )

  targetDirME = targetDir0E + "/MacOS"
  targetDirMV = targetDir0V + "/MacOS"
  targetDirRE = targetDir0E + "/Resources"
  targetDirRV = targetDir0V + "/Resources"

  os.makedirs(targetDirME)
  os.makedirs(targetDirMV)
  os.makedirs(targetDirRE)
  os.makedirs(targetDirRV)


  print( " [4] Copying script files and icon files ..." )
  #-------------------------------------------------------------------------------
  # [4] Copy different script files icon files
  #-------------------------------------------------------------------------------
  os.chdir(ProjectDir)
  resourceDir = "macbuild/Resources"

  shutil.copy2( resourceDir + "/KLayoutEditor.sh",  targetDirME )
  shutil.copy2( resourceDir + "/KLayoutViewer.sh",  targetDirMV )
  shutil.copy2( resourceDir + "/klayout-red.icns",  targetDirRE )
  shutil.copy2( resourceDir + "/klayout-blue.icns", targetDirRV )

  os.chmod( targetDirME + "/KLayoutEditor.sh",  0o0755 )
  os.chmod( targetDirMV + "/KLayoutViewer.sh",  0o0755 )
  os.chmod( targetDirRE + "/klayout-red.icns",  0o0644 )
  os.chmod( targetDirRV + "/klayout-blue.icns", 0o0644 )

  tmpfileE = ProjectDir + "/macbuild/Resources/Info.plist.template"
  keydicE  = { 'exe': 'KLayoutEditor.sh', 'icon': 'klayout-red.icns',  'bname': 'klayout', 'ver': Version }
  plistE   = GenerateInfoPlist( keydicE, tmpfileE )
  fileE    = open( targetDir0E + "/Info.plist", "w" )
  fileE.write(plistE)
  fileE.close()

  tmpfileV = ProjectDir + "/macbuild/Resources/Info.plist.template"
  keydicV  = { 'exe': 'KLayoutViewer.sh', 'icon': 'klayout-blue.icns', 'bname': 'klayout', 'ver': Version }
  plistV   = GenerateInfoPlist( keydicV, tmpfileV )
  fileV    = open( targetDir0V + "/Info.plist", "w" )
  fileV.write(plistV)
  fileV.close()
  print( "##### Finished deploying files for <KLayoutEditor.app> and <KLayoutViewer.app> #####" )
  print("")
  os.chdir(ProjectDir)
  return 0

#------------------------------------------------------------------------------
## The main function
#------------------------------------------------------------------------------
def main():
  SetGlobals()
  ParseCommandLineArguments()
  #----------------------------------------------------------
  # [The main build stage]
  #----------------------------------------------------------
  ret = RunMainBuildBash()
  if not DeploymentF and not DeploymentP:
    if not ret == 0:
      sys.exit(1)
  else:
    #----------------------------------------------------------
    # [Deployment stage-1]
    #   Deployment of dynamic link libraries, executables and
    #   resources to make the main "klayout.app" bundle
    #----------------------------------------------------------
    ret1 = DeployBinariesForBundle()
    if not ret1 == 0:
      sys.exit(1)
    #----------------------------------------------------------
    # [Deployment stage-2]
    #   Deployment of wrapper Bash scripts and resources
    #   to make "KLayoutEditor.app" and "KLayoutViewer.app"
    #----------------------------------------------------------
    ret2 = DeployScriptBundles()

    if not ret2 == 0:
      sys.exit(1)

#===================================================================================
if __name__ == "__main__":
  main()

#---------------
# End of file
#---------------
