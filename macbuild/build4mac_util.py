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
import re
import string
import subprocess
import shutil

try:
    from pathlib import Path
    Path('~').expanduser()
except (ImportError,AttributeError):  # python2
    from pathlib2 import Path

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
  dependent  = alllines[0].split(':')[0].strip()
  supporters = []
  for line in alllines[1:]:
    supporter = line.strip().split(' ')[0].strip()
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
      return 1

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
        return 1
  # for-lib
  return 0



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
  deplibs    = exedepdic[ list(keys)[0] ]

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
      return 1

    #-----------------------------------------------------------
    # [2] Make the application aware of the new identification
    #-----------------------------------------------------------
    nameOld = "%s" % lib
    nameNew = "@executable_path/%s/%s"  % ( relativedir, lib )
    command = "%s %s %s %s" % ( cmdNameChg, nameOld, nameNew, executable )
    if subprocess.call( command, shell=True ) != 0:
      msg = "!!! Failed to make the application aware of the new identification name <%s> !!!"
      print( msg % nameNew, file=sys.stderr )
      return 1
  # for-lib
  return 0

def WalkLibDependencyTree( dylibPath, depth=0, filter_regex=r'\t+/usr/local/opt'):
  NOTHINGTODO = []  # return empty list if nothing to do.
  dylibPath = str(Path(dylibPath))
  cmdNameId  = XcodeToolChain['nameID']
  cmdNameChg = XcodeToolChain['nameCH']
  otoolCm    = 'otool -L %s | grep -E "%s"' % (dylibPath, filter_regex)
  otoolOut   = os.popen( otoolCm ).read()
  exedepdic  = DecomposeLibraryDependency( dylibPath + ":\n" + otoolOut )
  keys       = exedepdic.keys()
  deplibs    = exedepdic[ list(keys)[0] ]

  if depth < 5:
    if len(deplibs) > 0:
      for idx, lib in enumerate(deplibs):
        lib = str(Path(lib))
        if lib != list(keys)[0]:
          deplibs[idx] = WalkLibDependencyTree(lib, depth+1, filter_regex)
    if depth == 0:
      return deplibs
    return exedepdic
  else:
    raise RuntimeError("Exceeded maximum recursion depth.")

def WalkFrameworkPaths(frameworkPaths, filter_regex=r'\.(so|dylib)$', search_path_filter=r'\t+/usr/local/opt'):

  if isinstance(frameworkPaths, str):
    frameworkPathsIter = [frameworkPaths]
  else:
    frameworkPathsIter = frameworkPaths

  dependency_dict = dict()

  for frameworkPath in frameworkPathsIter:
    frameworkPath = str(Path(frameworkPath))
    # print("Calling:", 'find %s -type f | grep -E "%s"' % (frameworkPath, filter_regex))
    find_grep_results = os.popen('find %s -type f | grep -E "%s"' % (frameworkPath, filter_regex)).read().split('\n')
    framework_files = filter(lambda x: x != '',
                          map(lambda x: x.strip(),
                                find_grep_results))

    dependency_dict[frameworkPath] = list()
    for idx, file in enumerate(framework_files):
      dict_file = {file: WalkLibDependencyTree(file, filter_regex=search_path_filter)}
      dependency_dict[frameworkPath].append(dict_file)
  return dependency_dict

def WalkDictTree(dependencyDict, visited_files):
  libNameChanges = list()
  for lib, dependencies in dependencyDict.items():
    if lib in visited_files:
      continue

    dependency_list = list()
    if isinstance(dependencies, list):
      for deplib in dependencies:
        if isinstance(deplib, str):
          dependency_list.append(deplib)
          if deplib not in visited_files:
            visited_files.append(deplib)
        elif isinstance(deplib, dict):
          dependency_list.append(str(next(iter(deplib))))
          libNameChanges.extend(WalkDictTree(deplib, visited_files))
        else:
          #raise RuntimeError("Unexpected value: %s" % deplib)
          pass
    else:
      raise RuntimeError("Unexpected value: %s" % dependencies)
    if len(dependency_list) > 0:
      libNameChanges.append((lib, dependency_list))
    else:
      libNameChanges.append((lib, ))
    visited_files.append(lib)
  return libNameChanges

def FindFramework(path, root_path):
  path = Path(path)
  root_path = Path(root_path)
  relPath = path.relative_to(root_path)
  return str(root_path / relPath.parts[0])

def ResolveExecutablePath(path, executable_path):
  """ Transforms @executable_path into executable_path"""
  executable_path = str(executable_path)
  p = Path(str(path).replace("@executable_path", "/%s/" % executable_path))
  return p

def DetectChanges(frameworkDependencyDict):
  visited_files = list()
  libNameChanges = list()
  for framework, libraries in frameworkDependencyDict.items():
    for libraryDict in libraries:
      libNameChanges.extend(WalkDictTree(libraryDict, visited_files))
  # Changes are stored in libNameChanges in the form of ('lib.dylib', ['dep1.dylib', ...])

  return libNameChanges

def PerformChanges(frameworkDependencyDict, replaceFromToPairs=None, executable_path="/tmp/klayout"):
  libNameChanges = DetectChanges(frameworkDependencyDict)
  cmdNameId = XcodeToolChain['nameID']
  cmdNameChg = XcodeToolChain['nameCH']

  if replaceFromToPairs is not None:
    for libNameChange in libNameChanges:
      libNameChangeIterator = iter(libNameChange)
      lib = next(libNameChangeIterator)
      try:
        dependencies = next(libNameChangeIterator)
      except StopIteration:
        dependencies = list()
      for replaceFrom, replaceTo, libdir in replaceFromToPairs:
        replaceFrom = str(Path(replaceFrom))
        replaceTo = str(Path(replaceTo))

        fileName = ResolveExecutablePath(lib.replace(replaceFrom, replaceTo), executable_path)
        if str(fileName).startswith('/usr'):
          # print(f'skipping fileName: {fileName}')
          continue

        if lib.find(replaceFrom) >= 0:
          if libdir:
            frameworkPath = FindFramework(lib, replaceFrom)
          else:
            frameworkPath = lib
          destFrameworkPath = frameworkPath.replace(replaceFrom, replaceTo)
          destFrameworkPath = ResolveExecutablePath(destFrameworkPath, executable_path)

          if not fileName.exists():
            print (lib.replace(replaceFrom, replaceTo), "DOES NOT EXIST")
            print ("COPY", frameworkPath, " -> ", destFrameworkPath)
            shutil.copytree(frameworkPath, str(destFrameworkPath))

          nameId = lib.replace(replaceFrom, replaceTo)
          command = "%s %s %s" % ( cmdNameId, nameId, fileName )
          if not os.access(str(fileName), os.W_OK):
            command = "chmod u+w %s; %s; chmod u-w %s" % (fileName, command, fileName)
          # print("\t%s" % command)
          if subprocess.call( command, shell=True ) != 0:
            msg = "!!! Failed to set the new identification name to <%s> !!!"
            print( msg % fileName, file=sys.stderr )
            return 1

        for dependency in dependencies:
          if dependency.find(replaceFrom) >= 0:
            print("\tIn:", fileName)
            print("\tRENAME", dependency, " -> ", dependency.replace(replaceFrom, replaceTo))

            # Try changing id first
            nameId = dependency.replace(replaceFrom, replaceTo)
            command = "%s %s %s" % ( cmdNameId, nameId, fileName)
            if not os.access(str(fileName), os.W_OK):
              command = "chmod u+w %s; %s; chmod u-w %s" % (fileName, command, fileName)
              # print("\t%s" % command)
            if subprocess.call( command, shell=True ) != 0:
              msg = "!!! Failed to set the new identification name to <%s> !!!"
              print( msg % fileName, file=sys.stderr )
              return 1

            # Rename dependencies
            nameOld = dependency
            nameNew = dependency.replace(replaceFrom, replaceTo)
            command = "%s %s %s %s" % ( cmdNameChg, nameOld, nameNew, str(fileName) )
            if not os.access(str(fileName), os.W_OK):
              command = "chmod u+w %s; %s; chmod u-w %s" % (fileName, command, fileName)

            # print("\t%s" % command)
            if subprocess.call( command, shell=True ) != 0:
              msg = "!!! Failed to set the new identification name to <%s> !!!"
              print( msg % fileName, file=sys.stderr )
              return 1


#------------------------------------------------------------------------------
## To get KLayout's version from a file; most likely from 'version.sh'
#
# @param[in] verfile   version file from which version is retrieved
#
# @return version string
#------------------------------------------------------------------------------
def GetKLayoutVersionFrom( verfile='version.h' ):
  version = "?.?.?"
  try:
    fd = open( verfile, "r" )
    contents = fd.readlines()
    fd.close()
  except Exception as e:
    return version

  verReg = re.compile( u'(KLAYOUT_VERSION=\")([0-9A-Z_a-z\.]+)(\")' )
  for line in contents:
    m = verReg.match(line)
    if m:
      # print(m.group(0)) # KLAYOUT_VERSION="0.25.1"
      # print(m.group(1)) # KLAYOUT_VERSION="
      # print(m.group(2)) # 0.25.1
      # print(m.group(3)) # "
      version = m.group(2)
      return version
  return version

#------------------------------------------------------------------------------
## To generate the contents of "Info.plist" file from a template
#
# @param[in]  keydic    dictionary of four key words ['exe', 'icon', 'bname', 'ver']
# @param[in]  templfile template file ("macbuild/Resources/Info.plist.template")
#
# @return generated strings
#------------------------------------------------------------------------------
def GenerateInfoPlist( keydic, templfile ):
  val_exe   = keydic['exe']
  val_icon  = keydic['icon']
  val_bname = keydic['bname']
  val_ver   = keydic['ver']

  try:
    fd = open( templfile, "r" )
    template = fd.read()
    fd.close()
  except Exception as e:
    return "???"

  t = string.Template(template)
  s = t.substitute( EXECUTABLE = val_exe,
                    ICONFILE   = val_icon,
                    BUNDLENAME = val_bname,
                    VERSION    = val_ver)
  return s

#----------------
# End of File
#----------------
