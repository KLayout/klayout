#! /usr/bin/env python
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac.py"
#
#  The top script for building KLayout (http://www.klayout.de/index.php)
#  version 0.25 or later on different Apple Mac OSX platforms.
#===============================================================================
from __future__ import print_function  # to use print() of Python 3 in Python >= 2.7
import sys
import os
import platform
import optparse
import subprocess

#-------------------------------------------------------------------------------
## To import global dictionaries of different modules
#-------------------------------------------------------------------------------
mydir = os.path.dirname(os.path.abspath(__file__))
sys.path.append( mydir + "/macbuild" )
from build4mac_env import *

#-------------------------------------------------------------------------------
## To set global variables including present directory and platform info.
#-------------------------------------------------------------------------------
def SetGlobals():
  global PresentDir         # present directory
  global Usage              # string on usage
  global BuildBash          # the main build Bash script
  global Platform           # platform
  global ModuleQt           # Qt module to be used
  global ModuleRuby         # Ruby module to be used
  global ModulePython       # Python module to be used
  global NoQtBindings       # True if not creating Qt bindings for Ruby scripts
  global MakeOptions        # options passed to `make`
  global DebugMode          # True if debug mode build
  global CheckComOnly       # True if check the command line only
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
  Usage += "   [-?|--?]             : print this usage and exit                       | disabled\n"
  Usage += "---------------------------------------------------------------------------------------------\n"

  PresentDir = os.getcwd()
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

  target  = "%s %s %s" % (Platform, Release, Machine)
  modules = "Qt=%s, Ruby=%s, Python=%s" % (ModuleQt, ModuleRuby, ModulePython)
  message = "### You are going to build KLayout\n    for  <%s>\n    with <%s>...\n"
  print("")
  print( message % (target, modules) )

#------------------------------------------------------------------------------
## To run the main Bash script with appropriate options
#------------------------------------------------------------------------------
def Run():
  global Platform
  global BuildBash
  global ModuleQt
  global ModuleRuby
  global ModulePython
  global NoQtBindings
  global MakeOptions
  global DebugMode
  global CheckComOnly
  global MacBinDir    # binary directory
  global MacBuildDir  # build directory
  global MacBuildLog  # build log file

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
    parameters += " \\\n  -qt4"
    parameters += " \\\n  -qmake  %s" % Qt4MacPorts['qmake']
    MacBinDir   = "./qt4.bin.macos-%s-%s"       % (Platform, mode)
    MacBuildDir = "./qt4.build.macos-%s-%s"     % (Platform, mode)
    MacBuildLog = "./qt4.build.macos-%s-%s.log" % (Platform, mode)
    parameters += " \\\n  -bin    %s" % MacBinDir
    parameters += " \\\n  -build  %s" % MacBuildDir
  elif ModuleQt == 'Qt5MacPorts':
    parameters += " \\\n  -qt5"
    parameters += " \\\n  -qmake  %s" % Qt5MacPorts['qmake']
    MacBinDir   = "./qt5.bin.macos-%s-%s"       % (Platform, mode)
    MacBuildDir = "./qt5.build.macos-%s-%s"     % (Platform, mode)
    MacBuildLog = "./qt5.build.macos-%s-%s.log" % (Platform, mode)
    parameters += " \\\n  -bin    %s" % MacBinDir
    parameters += " \\\n  -build  %s" % MacBuildDir

  # (C) want Qt bindings with Ruby scripts?
  if NoQtBindings:
    parameters += "\\\n  -without-qtbinding"
  else:
    parameters += "\\\n  -with-qtbinding"

  # (D) options to `make` tool
  if not MakeOptions == "":
    parameters += " \\\n  -option %s" % MakeOptions

  # (F) about Ruby
  if ModuleRuby == "nil":
    parameters += " \\\n  -noruby"
  else:
    parameters += " \\\n  -ruby   %s" % RubyDictionary[ModuleRuby]['exe']
    parameters += " \\\n  -rbinc  %s" % RubyDictionary[ModuleRuby]['inc']
    parameters += " \\\n  -rblib  %s" % RubyDictionary[ModuleRuby]['lib']

  # (G) about Python
  if ModulePython == "nil":
    parameters += " \\\n  -nopython"
  else:
    parameters += " \\\n  -python %s" % PythonDictionary[ModulePython]['exe']
    parameters += " \\\n  -pyinc  %s" % PythonDictionary[ModulePython]['inc']
    parameters += " \\\n  -pylib  %s" % PythonDictionary[ModulePython]['lib']

  #-----------------------------------------------------
  # [2] Make the consolidated command line
  #-----------------------------------------------------
  command  = "%s \\\n" % BuildBash
  command += parameters
  command += "  2>&1 | tee %s" % MacBuildLog
  if CheckComOnly:
    print(command)
    quit()

  #-----------------------------------------------------
  # [3] Invoke the main Bash script
  #-----------------------------------------------------
  myscript = "build4mac.py"
  if subprocess.call( command, shell=True ) != 0:
    print("")
    print( "-------------------------------------------------------------" )
    print( "!!! <%s>: failed to build KLayout" % myscript, file=sys.stderr )
    print( "-------------------------------------------------------------" )
    print("")
    sys.exit(1)
  else:
    print("")
    print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" )
    print( "### <%s>: successfully built KLayout" % myscript, file=sys.stderr )
    print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++" )
    print("")
    sys.exit(0)

#------------------------------------------------------------------------------
## The main function
#------------------------------------------------------------------------------
def main():
  SetGlobals()
  ParseCommandLineArguments()
  Run()

#===================================================================================
if __name__ == "__main__":
  main()

#---------------
# End of file
#---------------
