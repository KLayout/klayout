#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#========================================================================================
# File: "macbuild/build4mac_util.py"
#
# Here are utility functions and classes ...
#  for building KLayout (http://www.klayout.de/index.php)
#  version 0.26.1 or later on different Apple Mac OSX platforms.
#
# This file is imported by 'build4mac.py' script.
#========================================================================================
import sys
import os
import re
import string
import subprocess
import shutil

#----------------------------------------------------------------------------------------
## To import global dictionaries of different modules
#----------------------------------------------------------------------------------------
mydir = os.path.dirname(os.path.abspath(__file__))
sys.path.append( mydir )
from build4mac_env  import *

#----------------------------------------------------------------------------------------
## To decompose strings obtained by 'otool -L <*.dylib>' command and to
#  generate a dictionary of KLayout's inter-library dependency.
#
# @param[in] depstr   strings that tell dependency such as:
#
#  libklayout_edt.0.dylib:
#    libklayout_edt.0.dylib (compatibility version 0.26.0, current version 0.26.1)
#    libklayout_tl.0.dylib (compatibility version 0.26.0, current version 0.26.1)
#    libklayout_gsi.0.dylib (compatibility version 0.26.0, current version 0.26.1)
#    libklayout_laybasic.0.dylib (compatibility version 0.26.0, current version 0.26.1)
#    libklayout_db.0.dylib (compatibility version 0.26.0, current version 0.26.1)
#      :
#      :
#
# @return a dictionary
#----------------------------------------------------------------------------------------
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

#----------------------------------------------------------------------------------------
## To print the contents of a library dependency dictionary
#
# @param[in] depdic  dictionary
# @param[in] pathdic path dictionary
# @param[in] namedic dictionary name
#----------------------------------------------------------------------------------------
def PrintLibraryDependencyDictionary( depdic, pathdic, namedic ):
    keys = depdic.keys()
    print( "" )
    print( "##### Contents of <%s> #####:" % namedic )
    for key in keys:
        supporters = depdic[key]
        keyName = os.path.basename(key)
        print( " %s: (%s)" % (key, pathdic[keyName]) )
        for item in supporters:
            itemName = os.path.basename(item)
            if itemName != keyName and (itemName in pathdic):
                print( "    %s (%s)" % (item, pathdic[itemName]) )

#----------------------------------------------------------------------------------------
## To set and change identification name of KLayout's dylib
#
# @param[in] libdic  inter-library dependency dictionary
#
# @return 0 on success; non-zero on failure
#----------------------------------------------------------------------------------------
def SetChangeIdentificationNameOfDyLib( libdic, pathDic ):
    cmdNameId     = XcodeToolChain['nameID']
    cmdNameChg    = XcodeToolChain['nameCH']
    dependentLibs = libdic.keys()

    for lib in dependentLibs:
        #-----------------------------------------------------------
        # [1] Set the identification name of each dependent library
        #-----------------------------------------------------------
        nameOld = "%s" % lib
        libName = os.path.basename(lib)
        nameNew = pathDic[libName]
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
            supName = os.path.basename(sup)
            if libName != supName and (supName in pathDic):
                nameOld = "%s" % sup
                nameNew = pathDic[supName]
                command = "%s %s %s %s" % ( cmdNameChg, nameOld, nameNew, lib )
                if subprocess.call( command, shell=True ) != 0:
                    msg = "!!! Failed to make the library aware of the new identification name <%s> of supporter <%s> !!!"
                    print( msg % (nameNew, sup), file=sys.stderr )
                    return 1
    # for-lib
    return 0

#----------------------------------------------------------------------------------------
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
#
# @return 0 on success; non-zero on failure
#----------------------------------------------------------------------------------------
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
        #     $ install_name_tool [-id name] input
        #-----------------------------------------------------------
        nameOld = "klayout.app/Contents/Frameworks/%s" % lib  # input file
        nameNew = "@executable_path/%s/%s"  % ( relativedir, lib )
        command = "%s %s %s" % ( cmdNameId, nameNew, nameOld )
        if subprocess.call( command, shell=True ) != 0:
            msg = "!!! Failed to set the new identification name to <%s> !!!"
            print( msg % lib, file=sys.stderr )
            return 1

        #-----------------------------------------------------------
        # [2] Make the application aware of the new identification
        #     $ install_name_tool [-change old new] input
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

#----------------------------------------------------------------------------------------
## To make a library dependency dictionary by recursively walk down the lib hierarchy
#     DefaultHomebrewRoot = '/opt/homebrew' "arm64"  Apple Silicon
#     DefaultHomebrewRoot = '/usr/local'    "x86_64" Intel Mac
#
# @param[in] dylibPath:     dylib path
# @param[in] depth:         hierarchy depth (< 5)
# @param[in] filter_regex:  filter regular expression
#
# @return a dictionary
#----------------------------------------------------------------------------------------
def WalkLibDependencyTree( dylibPath, depth=0, filter_regex=r'\t+%s/opt' % DefaultHomebrewRoot ):
    otoolCm   = 'otool -L %s | grep -E "%s"' % (dylibPath, filter_regex)
    otoolOut  = os.popen( otoolCm ).read()
    exedepdic = DecomposeLibraryDependency( dylibPath + ":\n" + otoolOut )
    keys      = exedepdic.keys()
    deplibs   = exedepdic[ list(keys)[0] ]

    if depth < 5:
        if len(deplibs) > 0:
            for idx, lib in enumerate(deplibs):
                lib = str(lib)
                if lib != list(keys)[0]:
                    deplibs[idx] = WalkLibDependencyTree( lib, depth+1, filter_regex )
        if depth == 0:
            return deplibs
        return exedepdic
    else:
        raise RuntimeError( "Exceeded maximum recursion depth." )

#----------------------------------------------------------------------------------------
## To make a library dependency dictionary by recursively walk down the Framework
#     DefaultHomebrewRoot = '/opt/homebrew' "arm64"  Apple Silicon
#     DefaultHomebrewRoot = '/usr/local'    "x86_64" Intel Mac
#
# @param[in] frameworkPaths:      Framework path
# @param[in] filter_regex:        filter regular expression
# @param[in] search_path_filter:  search path filter regular expression
#
# @return a dictionary
#----------------------------------------------------------------------------------------
def WalkFrameworkPaths( frameworkPaths, filter_regex=r'\.(so|dylib)$',
                                        search_path_filter=r'\t+%s/opt' % DefaultHomebrewRoot ):
    if isinstance( frameworkPaths, str ):
        frameworkPathsIter = [frameworkPaths]
    else:
        frameworkPathsIter = frameworkPaths

    dependency_dict = dict()

    for frameworkPath in frameworkPathsIter:
        # print("Calling:", 'find %s -type f | grep -E "%s"' % (frameworkPath, filter_regex))
        find_grep_results = os.popen( 'find %s -type f | grep -E "%s"' % (frameworkPath, filter_regex) ).read().split('\n')
        framework_files = filter( lambda x: x != '', map(lambda x: x.strip(), find_grep_results) )

        dependency_dict[frameworkPath] = list()
        for idx, file in enumerate(framework_files):
            dict_file = { file: WalkLibDependencyTree( file, filter_regex=search_path_filter ) }
            dependency_dict[frameworkPath].append(dict_file)
    return dependency_dict

#----------------------------------------------------------------------------------------
## To make a list of changed libraries
#
# @param[in] dependencyDict:  library dependency dictionary
# @param[in] visited_files:   list of visited files
#
# @return a list
#----------------------------------------------------------------------------------------
def WalkDictTree( dependencyDict, visited_files ):
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
                    dependency_list.append( next(iter(deplib)) )
                    libNameChanges.extend( WalkDictTree(deplib, visited_files) )
                else:
                    #raise RuntimeError("Unexpected value: %s" % deplib)
                    pass
        else:
            raise RuntimeError( "Unexpected value: %s" % dependencies )
        if len(dependency_list) > 0:
            libNameChanges.append( (lib, dependency_list) )
        else:
            libNameChanges.append( (lib, ) )
        visited_files.append(lib)
    return libNameChanges

#----------------------------------------------------------------------------------------
## To find the Framework name from a library name
#
# @param[in] path:      path to a library
# @param[in] root_path: root path
#
# @return the path to a Framework
#----------------------------------------------------------------------------------------
def FindFramework( path, root_path ):
    relPath = os.path.relpath(path, root_path)
    frmPath = os.path.join(root_path, relPath.split(os.sep)[0])
    #print( "###", frmPath, path, root_path )
    return frmPath

#----------------------------------------------------------------------------------------
## To resolve an executable path
#
# @param[in] path:            a path to resolve
# @param[in] executable_path: an executable path
#
# @return the resolved path
#----------------------------------------------------------------------------------------
def ResolveExecutablePath( path, executable_path ):
    """ Transforms @executable_path into executable_path"""
    p = path.replace( "@executable_path", "/%s/" % executable_path )
    return p

#----------------------------------------------------------------------------------------
## To detect the changed library names
#
# @param[in] frameworkDependencyDict: framework dependency dictionary
#
# @return a list of changes, each of which is stored in the form of
#         * ('lib.dylib', ['dep1.dylib', ...])
#           OR
#         * ('lib.dylib',)
#----------------------------------------------------------------------------------------
def DetectChanges(frameworkDependencyDict):
    visited_files  = list()
    libNameChanges = list()
    for framework, libraries in frameworkDependencyDict.items():
        for libraryDict in libraries:
            libNameChanges.extend( WalkDictTree(libraryDict, visited_files) )
    return libNameChanges

#----------------------------------------------------------------------------------------
## To perform the required changes
#
# @param[in] frameworkDependencyDict: framework dependency dictionary
# @param[in] replaceFromToPairs:      (from, to)-pair for replacement
# @param[in] executable_path:         executable path
#
# @return 0 on success; > 0 on failure
#----------------------------------------------------------------------------------------
def PerformChanges( frameworkDependencyDict, replaceFromToPairs=None, executable_path="/tmp/klayout" ):
    libNameChanges = DetectChanges(frameworkDependencyDict)
    #print(libNameChanges)
    cmdNameId  = XcodeToolChain['nameID']
    cmdNameChg = XcodeToolChain['nameCH']

    if replaceFromToPairs is None:
        return 0
    else:
        for libNameChange in libNameChanges:
            libNameChangeIterator = iter(libNameChange)
            lib = next(libNameChangeIterator)
            try:
                dependencies = next(libNameChangeIterator)
            except StopIteration:
                dependencies = list()
            for replaceFrom, replaceTo, libdir in replaceFromToPairs:
                fileName = ResolveExecutablePath(lib.replace(replaceFrom, replaceTo), executable_path)
                if fileName.startswith('/usr'):
                    # print(f'skipping fileName: {fileName}')
                    continue

                if lib.find(replaceFrom) >= 0:
                    if libdir:
                        frameworkPath = FindFramework(lib, replaceFrom)
                    else:
                        frameworkPath = lib
                    destFrameworkPath = frameworkPath.replace(replaceFrom, replaceTo)
                    destFrameworkPath = ResolveExecutablePath(destFrameworkPath, executable_path)

                    if not os.path.exists(fileName):
                        print( "     NOT FOUND:", lib.replace(replaceFrom, replaceTo) )
                        print( "       COPYING:", frameworkPath, " -> ", destFrameworkPath )
                        shutil.copytree(frameworkPath, destFrameworkPath)

                    nameId = lib.replace(replaceFrom, replaceTo)
                    command = "%s %s %s" % ( cmdNameId, nameId, fileName )
                    if not os.access(fileName, os.W_OK):
                        command = "chmod u+w %s; %s; chmod u-w %s" % (fileName, command, fileName)
                    # print("\t%s" % command)
                    if subprocess.call( command, shell=True ) != 0:
                        msg = "!!! Failed to set the new identification name to <%s> !!!"
                        print( msg % fileName, file=sys.stderr )
                        return 1

                for dependency in dependencies:
                    if dependency.find(replaceFrom) >= 0:
                        print( "       IN:", fileName )
                        print( "         RENAMING:", dependency, " -> ", dependency.replace(replaceFrom, replaceTo) )

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
        return 0

#----------------------------------------------------------------------------------------
## To get KLayout's version from a file; most likely from 'version.sh'
#
# @param[in] verfile   version file from which version is retrieved
#
# @return version string
#----------------------------------------------------------------------------------------
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
            # print(m.group(0)) # KLAYOUT_VERSION="0.26.1"
            # print(m.group(1)) # KLAYOUT_VERSION="
            # print(m.group(2)) # 0.26.1
            # print(m.group(3)) # "
            version = m.group(2)
            return version
    return version

#----------------------------------------------------------------------------------------
## To generate the contents of "Info.plist" file from a template
#
# @param[in]  keydic    dictionary of four key words ['exe', 'icon', 'bname', 'ver']
# @param[in]  templfile template file ("macbuild/Resources/Info.plist.template")
#
# @return generated strings
#----------------------------------------------------------------------------------------
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
                      VERSION    = val_ver )
    return s

#----------------------------------------------------------------------------------------
## To patch 'Python' itself in Python Framework
#
#  A relatively new Python 3.x may depend on other dylib(s) under /usr/local/opt/.
#  This was first found in:
#    Catalina0{kazzz-s} klayout (1)% python3
#    Python 3.8.16 (default, Dec 12 2022, 14:07:09)
#    [Clang 12.0.0 (clang-1200.0.32.29)] on darwin
#    Type "help", "copyright", "credits" or "license" for more information.
#    >>>
#
#    Catalina0{kazzz-s} Current (2)% pwd
#    /usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/Current
#
#    Catalina0{kazzz-s} Current (3)% ll
#    total 5024
#    drwxr-xr-x  7 kazzz-s  admin      224 Dec 16 23:02 .
#    drwxr-xr-x  4 kazzz-s  admin      128 Dec 16 21:40 ..
#    lrwxr-xr-x  1 kazzz-s  admin       17 Dec 17 08:46 Headers -> include/python3.8
#    -rwxr-xr-x  1 kazzz-s  admin  2571960 Dec 12 23:08 Python
#    drwxr-xr-x  3 kazzz-s  admin       96 Dec 12 23:08 include
#    drwxr-xr-x  5 kazzz-s  admin      160 Dec 12 23:08 lib
#    drwxr-xr-x  3 kazzz-s  admin       96 Dec 12 23:08 share
#
#    Catalina0{kazzz-s} Current (4)% otool -L Python
#    Python:
#        /usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/Python (compatibility version 3.8.0, current version 3.8.0)
#        /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1675.129.0)
#    [*] /usr/local/opt/gettext/lib/libintl.8.dylib (compatibility version 12.0.0, current version 12.0.0)
#        /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.100.1)
#
#  This library dependency [*] has to be changed as shown below:
#    Catalina0{kazzz-s} Current (5)% install_name_tool -change \
#                                                       /usr/local/opt/gettext/lib/libintl.8.dylib \
#                                                       @executable_path/../Frameworks/libintl.8.dylib \
#                                                       Python
#
#    Catalina0{kazzz-s} Current (6)% otool -L Python
#    Python:
#        /usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/Python (compatibility version 3.8.0, current version 3.8.0)
#        /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1675.129.0)
#    [*] @executable_path/../Frameworks/libintl.8.dylib (compatibility version 12.0.0, current version 12.0.0)
#        /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.100.1)
#
#
# @param[in]  pythonFrameworkPath:  Python Framework path
# @param[in]  filter_regex:         filter regular expression
#
# @return 0 on succcess; non-zero on failure
#----------------------------------------------------------------------------------------
def Patch_Python_In_PythonFramework( pythonFrameworkPath, filter_regex=r'\t+%s/opt' % DefaultHomebrewRoot ):
    #----------------------------------------------------------------------
    # [1] Get Python's dependency
    #----------------------------------------------------------------------
    target    = "%s/Python" % pythonFrameworkPath
    otoolCm   = 'otool -L %s | grep -E "%s"' % (target, filter_regex)
    otoolOut  = os.popen( otoolCm ).read()
    exedepdic = DecomposeLibraryDependency( target + ":\n" + otoolOut )
    keys      = exedepdic.keys()
    deplibs   = exedepdic[ list(keys)[0] ]
    #  print(deplibs)
    # [ '/usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/Python',
    #   '/usr/local/opt/gettext/lib/libintl.8.dylib'
    # ]

    #----------------------------------------------------------------------
    # [2] Change the library name
    #----------------------------------------------------------------------
    cmdNameChg = XcodeToolChain['nameCH']

    for lib in deplibs:
        basename = os.path.basename(lib)
        if basename == "Python": # self
            continue
        else:
            nameOld = "%s" % lib
            nameNew = "@executable_path/../Frameworks/%s" % basename
            command = "%s %s %s %s" % ( cmdNameChg, nameOld, nameNew, target )
            if subprocess.call( command, shell=True ) != 0:
                msg = "!!! Failed to make 'Python' aware of the new identification name <%s> of supporter <%s> !!!"
                print( msg % (nameNew, lib), file=sys.stderr )
                return 1
    # for-lib
    return 0

#----------------
# End of File
#----------------
