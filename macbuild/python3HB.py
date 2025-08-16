#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#==============================================================================
# File: macbuild/python3HB.py
#
# Descriptions: A handy tool to setup the standardized directory structures
#               for Homebrew's Python 3.x
#==============================================================================
import os
import sys
import platform
import optparse

#------------------------------------------------------------------------------
# Set global variables
#------------------------------------------------------------------------------
def SetGlobals():
    global DefaultHomebrewRoot
    global Usage

    (System, Node, Release, MacVersion, Machine, Processor) = platform.uname()
    if Machine == "arm64": # Apple Silicon!
      DefaultHomebrewRoot = '/opt/homebrew'
    else:
      DefaultHomebrewRoot = '/usr/local'
    del System, Node, Release, MacVersion, Machine, Processor

    Usage  = "\n"
    Usage += "----------------------------------------------------------------------------------------\n"
    Usage += "<< Usage of 'python3HB.py' >>\n"
    Usage += "   to setup the standardized directory structures for Homebrew's Python 3.x on Mac\n"
    Usage += "\n"
    Usage += "   option & argument      : descriptions                              | default value\n"
    Usage += "   -------------------------------------------------------------------+---------------\n"
    Usage += "   <-v|--version <number>>: in ['3.11', '3.12','3.13']                | ''\n"
    Usage += "   [-u|-unlink]           : unlink only                               | disabled\n"
    Usage += "   [-?|--?]               : print this usage and exit                 | disabled\n"
    Usage += "----------------------------------------------------------------------+-----------------\n"

#------------------------------------------------------------------------------
# Parse the command line arguments
#------------------------------------------------------------------------------
def Parse_CLI_Args():
    global Version
    global UnlinkOnly

    p = optparse.OptionParser( usage=Usage )

    p.add_option( '-v', '--version',
                  dest='version',
                  help="python3 version=['3.11', '3.12', '3.13']" )

    p.add_option( '-u', '--unlink',
                  action='store_true',
                  dest='unlink',
                  default=False,
                  help='unlink only' )

    p.add_option( '-?', '--??',
                  action='store_true',
                  dest='checkusage',
                  default=False,
                  help='check usage' )

    p.set_defaults( version    = "",
                    unlink     = False,
                    checkusage = False )

    opt, args = p.parse_args()
    if (opt.checkusage):
        print(Usage)
        sys.exit(0)

    Version    = opt.version
    UnlinkOnly = opt.unlink
    if not Version in [ '3.11', '3.12', '3.13' ]:
        print( "! Unsupported Python 3 version <%s>" % Version )
        print(Usage)
        sys.exit(0)

#------------------------------------------------------------------------------
# Set the directory structures
#------------------------------------------------------------------------------
def SetDirectoryStructures():
    #----------------------------------------------------------
    # [1] Check the root directory of python@${Version}
    #----------------------------------------------------------
    root = "%s/opt/python@%s" % (DefaultHomebrewRoot, Version)
    if not os.path.isdir(root):
        print( "! Found no such a directory <%s>" % root )
        sys.exit(0)

    #----------------------------------------------------------
    # [2] Go to "lib/" and make
    #     Python.framework -> ../Frameworks/Python.framework/
    #----------------------------------------------------------
    os.chdir( root )
    os.chdir( "lib/" )
    try:
        os.remove( "Python.framework" )
    except FileNotFoundError:
        pass
    if not UnlinkOnly:
        os.symlink( "../Frameworks/Python.framework/", "Python.framework" )

    #----------------------------------------------------------
    # [3] Go to "bin/" and make
    #     ./python${version} -> python3
    #     ./pip${version}    -> pip3
    #----------------------------------------------------------
    os.chdir( root )
    os.chdir( "bin/" )
    try:
        os.remove( "python3" )
        os.remove( "pip3" )
    except FileNotFoundError:
        pass
    if not UnlinkOnly:
        os.symlink( "./python%s" % Version, "python3" )
        os.symlink( "./pip%s"    % Version, "pip3" )

    #----------------------------------------------------------
    # [4] Go to "Frameworks/Python.framework/" and delete
    #     three symbolic links
    #----------------------------------------------------------
    os.chdir( root )
    os.chdir( "Frameworks/Python.framework/" )
    try:
        os.remove( "Headers" )
        os.remove( "Resources" )
        os.remove( "Python" )
    except FileNotFoundError:
        pass

    #----------------------------------------------------------
    # [5] Go to "Versions/" and make
    #     Current -> ${Version}/
    #----------------------------------------------------------
    os.chdir( root )
    os.chdir( "Frameworks/Python.framework/Versions/" )
    try:
        os.remove( "Current" )
    except FileNotFoundError:
        pass
    if not UnlinkOnly:
        os.symlink( "%s/" % Version, "Current" )

    #----------------------------------------------------------
    # [6] Go to "Frameworks/Python.framework/" and make
    #     three symbolic links
    #----------------------------------------------------------
    if not UnlinkOnly:
        os.chdir( root )
        os.chdir( "Frameworks/Python.framework/" )
        os.symlink( "Versions/Current/Headers/",   "Headers" )
        os.symlink( "Versions/Current/Resources/", "Resources" )
        os.symlink( "Versions/Current/Python",     "Python" )

#------------------------------------------------------------------------------
# The main function
#------------------------------------------------------------------------------
def Main():
    SetGlobals()
    Parse_CLI_Args()
    SetDirectoryStructures()

#===================================================================================
if __name__ == "__main__":
    Main()

#---------------
# End of file
#---------------
