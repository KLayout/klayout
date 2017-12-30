#! /usr/bin/env python
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac_util.py"
#
# Here are utility functions and classes ...
#  for building KLayout (http://www.klayout.de/index.php)
#  version 0.25 or later on different Apple Mac OSX platforms.
#
# This file is imported by 'build4mac.py' script.
#===============================================================================
from __future__ import print_function  # to use print() of Python 3 in Python >= 2.7
import sys
import os
import subprocess

#-------------------------------------------------------------------------------
## To import global dictionaries of different modules
#-------------------------------------------------------------------------------
mydir = os.path.dirname(os.path.abspath(__file__))
sys.path.append( mydir )
from build4mac_env  import *

#------------------------------------------------------------------------------
## To decompose strings obtained by 'otool -L <*.dylib>' command and to
#  generate a dictionary of KLayout's inter-library dependency.
#
# @param[in] depstr   strings that tell dependency such as:
#
#  libklayout_edt.0.dylib:
#    libklayout_edt.0.dylib (compatibility version 0.25.0, current version 0.25.0)
#    libklayout_tl.0.dylib (compatibility version 0.25.0, current version 0.25.0)
#    libklayout_gsi.0.dylib (compatibility version 0.25.0, current version 0.25.0)
#    libklayout_laybasic.0.dylib (compatibility version 0.25.0, current version 0.25.0)
#    libklayout_db.0.dylib (compatibility version 0.25.0, current version 0.25.0)
#      :
#      :
#
# @return a dictionary
#------------------------------------------------------------------------------
def DecomposeLibraryDependency( depstr ):
  alllines   = depstr.split('\n')
  numlines   = len(alllines)
  dependent  = alllines[0].split(':')[0].strip(' ').strip('\t')
  supporters = []
  for i in range(1, numlines):
    supporter = alllines[i].split(' ')[0].strip(' ').strip('\t')
    if not supporter == '':
      supporters.append(supporter)
  return { dependent: supporters }

#------------------------------------------------------------------------------
## To print the contents of a library dependency dictionary
#
# @param[in] depdic  dictionary
# @param[in] namedic dictionary name
#------------------------------------------------------------------------------
def PrintLibraryDependencyDictionary( depdic, namedic ):
  keys = depdic.keys()
  print("")
  print("##### Contents of <%s> #####:" % namedic )
  for key in keys:
    supporters = depdic[key]
    print( " %s:" % key )
    for item in supporters:
      print( "    %s" % item )

#------------------------------------------------------------------------------
## To set and change identification name of KLayout's dylib
#
# @param[in] libdic  inter-library dependency dictionary
#
# @return 0 on success; non-zero on failure
#------------------------------------------------------------------------------
def SetChangeIdentificationNameOfDyLib( libdic ):
  cmdNameId     = XcodeToolChain['nameID']
  cmdNameChg    = XcodeToolChain['nameCH']
  dependentLibs = libdic.keys()

  for lib in dependentLibs:
    #-----------------------------------------------------------
    # [1] Set the identification name of each dependent library
    #-----------------------------------------------------------
    nameOld = "%s" % lib
    nameNew = "@executable_path/../Frameworks/%s" % lib
    command = "%s %s %s" % ( cmdNameId, nameNew, nameOld )
    if subprocess.call( command, shell=True ) != 0:
      msg = "!!! Failed to set the new identification name to <%s> !!!"
      print( msg % lib, file=sys.stderr )
      return(1)

    #-------------------------------------------------------------------------
    # [2] Make the library aware of the new identifications of all supporters
    #-------------------------------------------------------------------------
    supporters = libdic[lib]
    for sup in supporters:
      nameOld = "%s" % sup
      nameNew = "@executable_path/../Frameworks/%s" % sup
      command = "%s %s %s %s" % ( cmdNameChg, nameOld, nameNew, lib )
      if subprocess.call( command, shell=True ) != 0:
        msg = "!!! Failed to make the library aware of the new identification name <%s> of supporter <%s> !!!"
        print( msg % (nameNew, sup), file=sys.stderr )
        return(1)
  # for-lib
  return(0)

#------------------------------------------------------------------------------
## To set the identification names of KLayout's libraries to an executable
#     and make the application aware of the library locations
#
# @param[in] executable    path/to/executable -- (1)
# @param[in] relativedir   directory of dylib relative to executable -- (2)
#
# Example:  (1) "klayout.app/Contents/MacOS/klayout"
#           (2) "../Frameworks"
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
#
# @return 0 on success; non-zero on failure
#------------------------------------------------------------------------------
def SetChangeLibIdentificationName( executable, relativedir ):
  cmdNameId  = XcodeToolChain['nameID']
  cmdNameChg = XcodeToolChain['nameCH']
  otoolCm    = "otool -L %s | grep libklayout" % executable
  otoolOut   = os.popen( otoolCm ).read()
  exedepdic  = DecomposeLibraryDependency( executable + ":\n" + otoolOut )
  keys       = exedepdic.keys()
  deplibs    = exedepdic[ keys[0] ]

  for lib in deplibs:
    #-----------------------------------------------------------
    # [1] Set the identification names for the library
    #-----------------------------------------------------------
    nameOld = "klayout.app/Contents/Frameworks/%s" % lib
    nameNew = "@executable_path/%s/%s"  % ( relativedir, lib )
    command = "%s %s %s" % ( cmdNameId, nameNew, nameOld )
    if subprocess.call( command, shell=True ) != 0:
      msg = "!!! Failed to set the new identification name to <%s> !!!"
      print( msg % lib, file=sys.stderr )
      return(1)

    #-----------------------------------------------------------
    # [2] Make the application aware of the new identification
    #-----------------------------------------------------------
    nameOld = "%s" % lib
    nameNew = "@executable_path/%s/%s"  % ( relativedir, lib )
    command = "%s %s %s %s" % ( cmdNameChg, nameOld, nameNew, executable )
    if subprocess.call( command, shell=True ) != 0:
      msg = "!!! Failed to make the application aware of the new identification name <%s> !!!"
      print( msg % nameNew, file=sys.stderr )
      return(1)
  # for-lib
  return(0)

#----------------
# End of File
#----------------
