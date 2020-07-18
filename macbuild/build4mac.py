#!/usr/bin/env python
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac.py"
#
#  The top Python script for building KLayout (http://www.klayout.de/index.php)
#  version 0.26.1 or later on different Apple Mac OSX platforms.
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
  global PackagePrefix      # the package prefix: 'ST-', 'LW-', 'HW-', or 'EX-'
  global DeployVerbose      # -verbose=<0-3> level passed to 'macdeployqt' tool
  global Version            # KLayout's version
  global ModuleSet          # (Qt, Ruby, Python)-tuple
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
  Usage += "       for building KLayout 0.26.1 or later on different Apple Mac OSX platforms.\n"
  Usage += "\n"
  Usage += "$ [python] ./build4mac.py \n"
  Usage += "   option & argument    : descriptions (refer to 'macbuild/build4mac_env.py' for details)| default value\n"
  Usage += "   --------------------------------------------------------------------------------------+---------------\n"
  Usage += "   [-q|--qt <type>]     : case-insensitive type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3']    | qt5macports \n"
  Usage += "                        :   Qt5MacPorts: use Qt5 from MacPorts                           | \n"
  Usage += "                        :       Qt5Brew: use Qt5 from Homebrew                           | \n"
  Usage += "                        :       Qt5Ana3: use Qt5 from Anaconda3                          | \n"
  Usage += "   [-r|--ruby <type>]   : case-insensitive type=['nil', 'Sys', 'MP26', 'HB27', 'Ana3']   | sys \n"
  Usage += "                        :    nil: don't bind Ruby                                        | \n"
  Usage += "                        :    Sys: use OS-bundled Ruby [2.0 - 2.6] depending on OS        | \n"
  Usage += "                        :   MP26: use Ruby 2.6 from MacPorts                             | \n"
  Usage += "                        :   HB27: use Ruby 2.7 from Homebrew                             | \n"
  Usage += "                        :   Ana3: use Ruby 2.5 from Anaconda3                            | \n"
  Usage += "   [-p|--python <type>] : case-insensitive type=['nil', 'Sys', 'MP37', 'HB37', 'Ana3']   | sys \n"
  Usage += "                        :    nil: don't bind Python                                      | \n"
  Usage += "                        :    Sys: use OS-bundled Python 2.7 [ElCapitan -- Catalina]      | \n"
  Usage += "                        :   MP37: use Python 3.7 from MacPorts                           | \n"
  Usage += "                        :   HB37: use Python 3.7 from Homebrew                           | \n"
  Usage += "                        :   Ana3: use Python 3.7 from Anaconda3                          | \n"
  Usage += "   [-n|--noqtbinding]   : don't create Qt bindings for ruby scripts                      | disabled \n"
  Usage += "   [-m|--make <option>] : option passed to 'make'                                        | '-j4' \n"
  Usage += "   [-d|--debug]         : enable debug mode build                                        | disabled \n"
  Usage += "   [-c|--checkcom]      : check command line and exit without building                   | disabled \n"
  Usage += "   [-y|--deploy]        : deploy executables and dylibs including Qt's Frameworks        | disabled \n"
  Usage += "   [-Y|--DEPLOY]        : deploy executables and dylibs for those who built KLayout      | disabled \n"
  Usage += "                        : from the source code and use the tools in the same machine     | \n"
  Usage += "                        : ! After confirmation of successful build of 'klayout.app',     | \n"
  Usage += "                        :   rerun this script with BOTH:                                 | \n"
  Usage += "                        :     1) the same options used for building AND                  | \n"
  Usage += "                        :     2) <-y|--deploy> OR <-Y|--DEPLOY>                          | \n"
  Usage += "                        :   optionally with [-v|--verbose <0-3>]                         | \n"
  Usage += "   [-v|--verbose <0-3>] : verbose level of `macdeployqt' (effective with -y only)        | 1 \n"
  Usage += "                        : 0 = no output, 1 = error/warning (default),                    | \n"
  Usage += "                        : 2 = normal,    3 = debug                                       | \n"
  Usage += "   [-?|--?]             : print this usage and exit                                      | disabled \n"
  Usage += "-----------------------------------------------------------------------------------------+---------------\n"

  ProjectDir = os.getcwd()
  BuildBash  = "./build.sh"
  (System, Node, Release, Version, Machine, Processor) = platform.uname()

  if not System == "Darwin":
    print("")
    print( "!!! Sorry. Your system <%s> looks like non-Mac" % System, file=sys.stderr )
    print(Usage)
    sys.exit(1)

  release = int( Release.split(".")[0] ) # take the first of ['19', '0', '0']
  if   release == 19:
    Platform = "Catalina"
  elif release == 18:
    Platform = "Mojave"
  elif release == 17:
    Platform = "HighSierra"
  elif release == 16:
    Platform = "Sierra"
  elif release == 15:
    Platform = "ElCapitan"
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

  # Set the default modules
  ModuleQt = "Qt5MacPorts"
  if   Platform == "Catalina":
    ModuleRuby   = "RubyCatalina"
    ModulePython = "PythonCatalina"
  elif Platform == "Mojave":
    ModuleRuby   = "RubyMojave"
    ModulePython = "PythonMojave"
  elif Platform == "HighSierra":
    ModuleRuby   = "RubyHighSierra"
    ModulePython = "PythonHighSierra"
  elif Platform == "Sierra":
    ModuleRuby   = "RubySierra"
    ModulePython = "PythonSierra"
  elif Platform == "ElCapitan":
    ModuleRuby   = "RubyElCapitan"
    ModulePython = "PythonElCapitan"
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
  PackagePrefix = ""
  DeployVerbose = 1
  Version       = GetKLayoutVersionFrom( "./version.sh" )
  ModuleSet     = ( 'qt5MP', 'Sys', 'Sys' )

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
  global PackagePrefix
  global DeployVerbose
  global ModuleSet

  p = optparse.OptionParser( usage=Usage )
  p.add_option( '-q', '--qt',
                dest='type_qt',
                help="Qt type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3']" )

  p.add_option( '-r', '--ruby',
                dest='type_ruby',
                help="Ruby type=['nil', 'Sys', 'MP26', 'HB27', 'Ana3']" )

  p.add_option( '-p', '--python',
                dest='type_python',
                help="Python type=['nil', 'Sys', 'MP37', 'HB37', 'Ana3']" )

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

  # Determine the Qt type
  candidates                = dict()
  candidates['QT5MACPORTS'] = 'Qt5MacPorts'
  candidates['QT5BREW']     = 'Qt5Brew'
  candidates['QT5ANA3']     = 'Qt5Ana3'
  try:
    ModuleQt = candidates[ opt.type_qt.upper() ]
  except KeyError:
    ModuleQt = ''
    pass
  if ModuleQt == '':
    print("")
    print( "!!! Unknown Qt type <%s>. Case-insensitive candidates: %s" % \
            (opt.type_qt, list(candidates.keys())), file=sys.stderr )
    print(Usage)
    sys.exit(1)
  elif ModuleQt == "Qt5MacPorts":
    choiceQt5 = 'qt5MP'
  elif ModuleQt == "Qt5Brew":
    choiceQt5 = 'qt5Brew'
  elif ModuleQt == "Qt5Ana3":
    choiceQt5 = 'qt5Ana3'

  # By default, OS-standard (-bundled) script languages (Ruby and Python) are used
  NonOSStdLang = False

  # Determine the Ruby type
  candidates         = dict()
  candidates['NIL']  = 'nil'
  candidates['SYS']  = 'Sys'
  candidates['MP26'] = 'MP26'
  candidates['HB27'] = 'HB27'
  candidates['ANA3'] = 'Ana3'
  try:
    choiceRuby = candidates[ opt.type_ruby.upper() ]
  except KeyError:
    ModuleRuby = ''
    pass
  else:
    ModuleRuby = ''
    if choiceRuby == "nil":
      ModuleRuby = 'nil'
    elif choiceRuby == "Sys":
      choiceRuby = "Sys"
      if Platform == "Catalina":
        ModuleRuby = 'RubyCatalina'
      elif Platform == "Mojave":
        ModuleRuby = 'RubyMojave'
      elif Platform == "HighSierra":
        ModuleRuby = 'RubyHighSierra'
      elif Platform == "Sierra":
        ModuleRuby = 'RubySierra'
      elif Platform == "ElCapitan":
        ModuleRuby = 'RubyElCapitan'
    elif choiceRuby == "MP26":
      ModuleRuby   = 'Ruby26MacPorts'
      NonOSStdLang = True
    elif choiceRuby == "HB27":
      ModuleRuby   = 'Ruby27Brew'
      NonOSStdLang = True
    elif choiceRuby == "Ana3":
      ModuleRuby   = 'RubyAnaconda3'
      NonOSStdLang = True
  if ModuleRuby == '':
    print("")
    print( "!!! Unknown Ruby type <%s>. Case-insensitive candidates: %s" % \
            (opt.type_ruby, list(candidates.keys())), file=sys.stderr )
    print(Usage)
    sys.exit(1)

  # Determine the Python type
  candidates         = dict()
  candidates['NIL']  = 'nil'
  candidates['SYS']  = 'Sys'
  candidates['MP37'] = 'MP37'
  candidates['HB37'] = 'HB37'
  candidates['ANA3'] = 'Ana3'
  try:
    choicePython = candidates[ opt.type_python.upper() ]
  except KeyError:
    ModulePython = ''
    pass
  else:
    ModulePython = ''
    if choicePython ==  "nil":
      ModulePython = 'nil'
    elif choicePython == "Sys":
      if Platform == "Catalina":
        ModulePython = 'PythonCatalina'
      elif Platform == "Mojave":
        ModulePython = 'PythonMojave'
      elif Platform == "HighSierra":
        ModulePython = 'PythonHighSierra'
      elif Platform == "Sierra":
        ModulePython = 'PythonSierra'
      elif Platform == "ElCapitan":
        ModulePython = 'PythonElCapitan'
    elif choicePython == "MP37":
      ModulePython = 'Python37MacPorts'
      NonOSStdLang = True
    elif choicePython == "HB37":
      ModulePython = 'Python37Brew'
      NonOSStdLang = True
    elif choicePython == "Ana3":
      ModulePython = 'PythonAnaconda3'
      NonOSStdLang = True
  if ModulePython == '':
    print("")
    print( "!!! Unknown Python type <%s>. Case-insensitive candidates: %s" % \
            (opt.type_python, list(candidates.keys())), file=sys.stderr )
    print(Usage)
    sys.exit(1)

  # Set of modules chosen
  ModuleSet = ( choiceQt5, choiceRuby, choicePython )

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
  else:
    message = "### You are going to make "
    if DeploymentP:
      PackagePrefix = "LW-"
      message      += "a llightweight (LW-) package excluding Qt5, Ruby, and Python..."
    elif DeploymentF:
      if (ModuleRuby in RubySys) and (ModulePython in PythonSys):
        PackagePrefix = "ST-"
        message      += "a standard (ST-) package including Qt5 and using OS-bundled Ruby and Python..."
      elif ModulePython == 'Python37Brew':
        PackagePrefix = "HW-"
        message      += "a heavyweight (HW-) package including Qt5 and Python3.7 from Homebrew..."
      else:
        PackagePrefix = "EX-"
        message      += "a package with exceptional (EX-) combinations of different modules..."
    print( "" )
    print( message )
    print( "" )
    if CheckComOnly:
      sys.exit(0)

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
  global ModuleSet
  global NoQtBindings
  global MakeOptions
  global DebugMode
  global CheckComOnly
  global DeploymentF
  global DeploymentP
  global PackagePrefix
  global MacPkgDir          # relative path to package directory
  global MacBinDir          # relative path to binary directory
  global MacBuildDir        # relative path to build directory
  global MacBuildDirQAT     # relative path to build directory for QATest
  global MacBuildLog        # relative path to build log file
  global AbsMacPkgDir       # absolute path to package directory
  global AbsMacBinDir       # absolute path to binary directory
  global AbsMacBuildDir     # absolute path to build directory
  global AbsMacBuildDirQAT  # absolute path to build directory for QATest
  global AbsMacBuildLog     # absolute path to build log file

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

  # (B) Modules
  (qt, ruby, python) = ModuleSet  # ( 'qt5MP', 'Sys', 'Sys' )
  ruby_python = "R%sP%s" % ( ruby.lower(), python.lower() )

  # (C) Target directories and files
  MacPkgDir         = "./%s%s.pkg.macos-%s-%s-%s"      % (PackagePrefix, qt, Platform, mode, ruby_python)
  MacBinDir         = "./%s.bin.macos-%s-%s-%s"        % (               qt, Platform, mode, ruby_python)
  MacBuildDir       = "./%s.build.macos-%s-%s-%s"      % (               qt, Platform, mode, ruby_python)
  MacBuildLog       = "./%s.build.macos-%s-%s-%s.log"  % (               qt, Platform, mode, ruby_python)
  AbsMacPkgDir      = "%s/%s%s.pkg.macos-%s-%s-%s"     % (ProjectDir, PackagePrefix, qt, Platform, mode, ruby_python)
  AbsMacBinDir      = "%s/%s.bin.macos-%s-%s-%s"       % (ProjectDir,                qt, Platform, mode, ruby_python)
  AbsMacBuildDir    = "%s/%s.build.macos-%s-%s-%s"     % (ProjectDir,                qt, Platform, mode, ruby_python)
  AbsMacBuildLog    = "%s/%s.build.macos-%s-%s-%s.log" % (ProjectDir,                qt, Platform, mode, ruby_python)
  MacBuildDirQAT    = MacBuildDir    + ".macQAT"
  AbsMacBuildDirQAT = AbsMacBuildDir + ".macQAT"

  # (D) Qt5
  if ModuleQt == 'Qt5MacPorts':
    parameters    += " \\\n  -qt5"
    parameters    += " \\\n  -qmake  %s" % Qt5MacPorts['qmake']
    parameters    += " \\\n  -bin    %s" % MacBinDir
    parameters    += " \\\n  -build  %s" % MacBuildDir
  elif ModuleQt == 'Qt5Brew':
    parameters    += " \\\n  -qt5"
    parameters    += " \\\n  -qmake  %s" % Qt5Brew['qmake']
    parameters    += " \\\n  -bin    %s" % MacBinDir
    parameters    += " \\\n  -build  %s" % MacBuildDir
  elif ModuleQt == 'Qt5Ana3':
    parameters    += " \\\n  -qt5"
    parameters    += " \\\n  -qmake  %s" % Qt5Ana3['qmake']
    parameters    += " \\\n  -bin    %s" % MacBinDir
    parameters    += " \\\n  -build  %s" % MacBuildDir
  parameters += " \\\n  -rpath    %s" % "@executable_path/../Frameworks"

  # (E) want Qt bindings with Ruby scripts?
  if NoQtBindings:
    parameters += " \\\n  -without-qtbinding"
  else:
    parameters += " \\\n  -with-qtbinding"

  # (F) options to `make` tool
  if not MakeOptions == "":
    parameters += " \\\n  -option %s" % MakeOptions

  # (G) about Ruby
  if ModuleRuby == "nil":
    parameters += " \\\n  -noruby"
  else:
    parameters += " \\\n  -ruby   %s" % RubyDictionary[ModuleRuby]['exe']
    parameters += " \\\n  -rbinc  %s" % RubyDictionary[ModuleRuby]['inc']
    parameters += " \\\n  -rblib  %s" % RubyDictionary[ModuleRuby]['lib']

  # (H) about Python
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

      #------------------------------------------------------------------------
      # [4] Prepare "*.macQAT/" directory for the QATest.
      #     Binaries under "*.macQAT/" such as *.dylib will be touched later.
      #------------------------------------------------------------------------
      print( "### Preparing <%s>" % MacBuildDirQAT )
      if os.path.isdir( MacBuildDirQAT ):
        shutil.rmtree( MacBuildDirQAT )

      os.chdir( MacBuildDir )
      tarFile = "../macQATest.tar"
      tarCmdC = "tar cf %s ." % tarFile
      if subprocess.call( tarCmdC, shell=True ) != 0:
        print( "", file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "!!! <%s>: failed to create <%s>" % (myscript, tarFile), file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "", file=sys.stderr )
        return 1

      os.chdir( "../" )
      os.mkdir( MacBuildDirQAT )
      os.chdir( MacBuildDirQAT )
      tarCmdX = "tar xf %s" % tarFile
      if subprocess.call( tarCmdX, shell=True ) != 0:
        print( "", file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "!!! <%s>: failed to unpack <%s>" % (myscript, tarFile), file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "", file=sys.stderr )
        return 1

      os.remove( tarFile )
      os.chdir( "../" )
      shutil.copy2( "macbuild/macQAT.sh", MacBuildDirQAT )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "### <%s>: prepared the initial *.macQAT/" % myscript, file=sys.stderr )
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
  global MacBuildDirQAT
  global MacBuildLog
  global AbsMacPkgDir
  global AbsMacBinDir
  global AbsMacBuildDir
  global AbsMacBuildDirQAT
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
  #                             |              +-- 'db_plugins' --slink--> ../MacOS/db_plugins/
  #                             +-- MacOS/+
  #                             |         +-- 'klayout'
  #                             |         +-- db_plugins/
  #                             |         +-- lay_plugins/
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
  #       (1) libklayout_lay.0.26.1.dylib
  #       (2) libklayout_lay.0.26.dylib -> libklayout_lay.0.26.1.dylib
  #       (3) libklayout_lay.0.dylib -> libklayout_lay.0.26.1.dylib
  #       (4) libklayout_lay.dylib -> libklayout_lay.0.26.1.dylib
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
  #   MacBookPro2(1)$ otool -L klayout
  #   klayout:
  #       :
  #       :
  #     libklayout_tl.0.dylib (compatibility version 0.26.0, current version 0.26.1)
  #     libklayout_gsi.0.dylib (compatibility version 0.26.0, current version 0.26.1)
  #     libklayout_db.0.dylib (compatibility version 0.26.0, current version 0.26.1)
  #       :
  #-------------------------------------------------------------------------------
  os.chdir( targetDirF )
  dynamicLinkLibs = glob.glob( os.path.join( AbsMacBinDir, "*.dylib" ) )
  depDicOrdinary  = {} # inter-library dependency dictionary
  pathDic = {} # paths to insert for each library
  for item in dynamicLinkLibs:
    if os.path.isfile(item) and not os.path.islink(item):
      #-------------------------------------------------------------------
      # (A) Copy an ordinary *.dylib file here by changing the name
      #     to style (3) and set its mode to 0755 (sanity check).
      #-------------------------------------------------------------------
      fullName = os.path.basename(item).split('.')
      # e.g. [ 'libklayout_lay', '0', '26', '1', 'dylib' ]
      nameStyle3 = fullName[0] + "." + fullName[1] + ".dylib"
      shutil.copy2( item, nameStyle3 )
      os.chmod( nameStyle3, 0o0755 )
      #-------------------------------------------------------------------
      # (B) Then get inter-library dependencies
      #     Note that will pull all dependencies and sort them out later
      #     dropping those which don't have a path entry
      #-------------------------------------------------------------------
      otoolCm   = "otool -L %s | grep dylib" % nameStyle3
      otoolOut  = os.popen( otoolCm ).read()
      dependDic = DecomposeLibraryDependency(otoolOut)
      depDicOrdinary.update(dependDic)
      #-------------------------------------------------------------------
      # (C) This library goes into Frameworks, hence record it's path there
      #-------------------------------------------------------------------
      pathDic[nameStyle3] = "@executable_path/../Frameworks/" + nameStyle3

  os.chdir(ProjectDir)
  #-------------------------------------------------------------------
  # Copy the contents of the plugin directories to a place next to
  # the application binary
  #-------------------------------------------------------------------
  for piDir in [ "db_plugins", "lay_plugins" ]:
    os.makedirs( os.path.join( targetDirM, piDir ))
    dynamicLinkLibs = glob.glob( os.path.join( MacBinDir, piDir, "*.dylib" ) )
    for item in dynamicLinkLibs:
      if os.path.isfile(item) and not os.path.islink(item):
        #-------------------------------------------------------------------
        # (A) Copy an ordinary *.dylib file here by changing the name
        #     to style (3) and set its mode to 0755 (sanity check).
        #-------------------------------------------------------------------
        fullName = os.path.basename(item).split('.')
        # e.g. [ 'libklayout_lay', '0', '26', '1', 'dylib' ]
        nameStyle3 = fullName[0] + "." + fullName[1] + ".dylib"
        destPath = os.path.join( targetDirM, piDir, nameStyle3 )
        shutil.copy2( item, destPath )
        os.chmod( destPath, 0o0755 )
        #-------------------------------------------------------------------
        # (B) Then get inter-library dependencies
        #     Note that will pull all dependencies and sort them out later
        #     dropping those which don't have a path entry
        #-------------------------------------------------------------------
        otoolCm   = "otool -L %s | grep 'dylib'" % destPath
        otoolOut  = os.popen( otoolCm ).read()
        dependDic = DecomposeLibraryDependency(otoolOut)
        depDicOrdinary.update(dependDic)
        #-------------------------------------------------------------------
        # (C) This library goes into the plugin dir
        #-------------------------------------------------------------------
        pathDic[nameStyle3] = "@executable_path/" + piDir + "/" + nameStyle3

  '''
  PrintLibraryDependencyDictionary( depDicOrdinary, pathDic, "Style (3)" )
  exit()
  '''

  #----------------------------------------------------------------------------------
  # (D) Make a symbolic link
  #        'db_plugins' --slink--> ../MacOS/db_plugins/
  #     under Frameworks/, which is required for the command line Buddy tools.
  #
  #     Ref. https://github.com/KLayout/klayout/issues/460#issuecomment-571803458
  #
  #             :
  #             +-- Frameworks/+
  #             |              +-- '*.framework'
  #             |              +-- '*.dylib'
  #             |              +-- 'db_plugins' --slink--> ../MacOS/db_plugins/
  #             +-- MacOS/+
  #             |         +-- 'klayout'
  #             |         +-- db_plugins/
  #             |         +-- lay_plugins/
  #             :
  #----------------------------------------------------------------------------------
  os.chdir( targetDirF )
  os.symlink( "../MacOS/db_plugins/", "./db_plugins" )


  print( " [5] Setting and changing the identification names among KLayout's libraries ..." )
  #-------------------------------------------------------------
  # [5] Set the identification names for KLayout's libraries
  #     and make the library aware of the locations of libraries
  #     on which it depends; that is, inter-library dependency
  #-------------------------------------------------------------
  ret = SetChangeIdentificationNameOfDyLib( depDicOrdinary, pathDic )
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
    # To use Qt5 from Homebrew on Catalina...
    #   in "/usr/local/opt/python/lib/"
    #         Python.framework -> ../Frameworks/Python.framework/ <=== this symbolic was needed
    #         pkgconfig/
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
      msg = "!!! Failed to deploy applications on OSX/macOS !!!"
      print( msg, file=sys.stderr )
      print("")
      os.chdir(ProjectDir)
      return 1

    #-------------------------------------------------------------
    # [9] Special deployment of Python3.7 from Homebrew
    #-------------------------------------------------------------
    deploymentPython37HB = (ModulePython == 'Python37Brew')
    if deploymentPython37HB and NonOSStdLang:
      from build4mac_util import WalkFrameworkPaths, PerformChanges

      bundlePath = AbsMacPkgDir + '/klayout.app'
      bundleExecPathAbs = '%s/Contents/MacOS/' % bundlePath
      pythonFrameworkPath = '%s/Contents/Frameworks/Python.framework' % bundlePath

      print( "" )
      print( " [9] Optional deployment of Python from %s ..." % HBPython37FrameworkPath )
      print( "  [9.1] Copying Python Framework" )

      cmd1 = "rm -rf %s" % pythonFrameworkPath
      cmd2 = "rsync -a --safe-links %s/ %s" % (HBPython37FrameworkPath, pythonFrameworkPath)
      cmd3 = "mkdir %s/Versions/3.7/lib/python3.7/site-packages/" % pythonFrameworkPath
      cmd4 = "cp -RL %s/Versions/3.7/lib/python3.7/site-packages/{pip*,pkg_resources,setuptools*,wheel*} " % \
              HBPython37FrameworkPath
      cmd4 += "%s/Versions/3.7/lib/python3.7/site-packages/" % pythonFrameworkPath
      cmd5 = "rm -rf %s/Versions/3.7/lib/python3.7/test" % pythonFrameworkPath
      cmd6 = "rm -rf %s/Versions/3.7/Resources" % pythonFrameworkPath
      cmd7 = "rm -rf %s/Versions/3.7/bin" % pythonFrameworkPath

      shell_commands = list()
      shell_commands.append(cmd1)
      shell_commands.append(cmd2)
      shell_commands.append(cmd3)
      shell_commands.append(cmd4)
      shell_commands.append(cmd5)
      shell_commands.append(cmd6)
      shell_commands.append(cmd7)

      for command in shell_commands:
        if subprocess.call( command, shell=True ) != 0:
          msg = "command failed: %s"
          print( msg % command, file=sys.stderr )
          sys.exit(1)

      shutil.copy2( sourceDir2 + "/start-console.py", targetDirM )
      shutil.copy2( sourceDir2 + "/klayout_console", targetDirM )
      os.chmod( targetDirM + "/klayout_console",      0o0755 )

      print("  [9.2] Relinking dylib dependencies inside Python.framework" )
      print("   [9.2.1] Patching Python Framework" )
      depdict = WalkFrameworkPaths( pythonFrameworkPath )
      appPythonFrameworkPath = '@executable_path/../Frameworks/Python.framework/'
      PerformChanges(depdict, [(HBPython37FrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs)

      print("   [9.2.2] Patching /usr/local/opt/ libs")
      usrLocalPath = '/usr/local/opt/'
      appUsrLocalPath = '@executable_path/../Frameworks/'
      replacePairs = [(usrLocalPath, appUsrLocalPath, True)]
      depdict = WalkFrameworkPaths(pythonFrameworkPath, search_path_filter=r'\t+/usr/local/(opt|Cellar)')
      PerformChanges(depdict, replacePairs, bundleExecPathAbs)

      print("   [9.2.3] Patching openssl, gdbm, readline, sqlite, tcl-tk, xz")
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

      print("  [9.3] Relinking dylib dependencies for klayout")
      klayoutPath = bundleExecPathAbs
      depdict = WalkFrameworkPaths(klayoutPath, filter_regex=r'klayout$')
      PerformChanges(depdict, [(HBPython37FrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs)

      libKlayoutPath = bundleExecPathAbs + '../Frameworks'
      depdict = WalkFrameworkPaths(libKlayoutPath, filter_regex=r'libklayout')
      PerformChanges(depdict, [(HBPython37FrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs)

      print("  [9.4] Patching site.py, pip/, and distutils/")
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

    #-------------------------------------------------------------
    # [10] Special deployment of Ruby2.6 from Homebrew?
    #-------------------------------------------------------------
    deploymentRuby26HB = (ModuleRuby == 'Ruby27Brew')
    if deploymentRuby26HB and NonOSStdLang:

      print( "" )
      print( " [10] You have reached optional deployment of Ruby from %s ..." % HBRuby27Path )
      print( "   [!!!] Sorry, the deployed package will not work properly since deployment of" )
      print( "         Ruby2.6 from Homebrew is not yet supported." )
      print( "         Since you have Homebrew development environment, there two options:" )
      print( "           (1) Retry to make a package with '-Y|--DEPLOY' option." )
      print( "               This will not deploy any of Qt5, Python, and Ruby from Homebrew." )
      print( "               Instead, the package will directly use those Frameworks and libraries" )
      print( "               in your Homebrew environment." )
      print( "           (2) Rebuild KLayout with '-r|--ruby <nil|Sys>' option depending on your preference." )
      print( "" )

  else:
    print( " [8] Skipped deploying Qt's Frameworks and optional Python/Ruby Frameworks..." )
  print( "##### Finished deploying libraries and executables for <klayout.app> #####" )
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
    # [The deployment stage]
    #   Deployment of dynamic link libraries, executables and
    #   resources to make the main "klayout.app" bundle
    #----------------------------------------------------------
    ret = DeployBinariesForBundle()
    if not ret == 0:
      sys.exit(1)

#===================================================================================
if __name__ == "__main__":
  main()

#---------------
# End of file
#---------------
