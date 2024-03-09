#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#=============================================================================================
# File: "macbuild/makeDMG4mac.py"
#
#  Python script for making a DMG file of KLayout (http://www.klayout.de/index.php) bundle.
#
#  This is a derivative work of Ref. 2) below. Refer to "macbuild/LICENSE" file.
#  Ref.
#   1) https://el-tramo.be/guides/fancy-dmg/
#   2) https://github.com/andreyvit/create-dmg.git
#=============================================================================================
from time import sleep
import sys
import os
import re
import shutil
import zipfile
import glob
import platform
import optparse
import subprocess
import hashlib
import string

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
    global LatestOS           # the latest generic OS name
    global GenOSName          # generic OS name
    global Platform           # platform
    global PkgDir             # the package directory where "klayout.app" exists
    global UnsafePkg          # flags whether to proceed to making "invalid" dmg
    global OpClean            # 'clean' operation
    global OpMake             # 'make' operation
    global DefaultBundleName  # the default bundle name 'klayout.app'
    global BundleName         # application bundle name in the DMG
    global DMGSerialNum       # the DMG serial number
    global PackagePrefix      # the package prefix: LW-', 'HW-', or 'EX-'
    global QtIdentification   # Qt identification
    global RubyPythonID       # Ruby- and Python-identification
    global KLVersion          # KLayout's version
    global OccupiedDS         # approx. occupied disc space
    global BackgroundPNG      # the background PNG image file
    global VolumeIcons        # the volume icon file
    global AppleScriptDMG     # the AppleScript for KLayout DMG
    global WorkDMG            # work DMG file created under ProjectDir/
    global VolumeDMG          # the volume name of DMG
    global TargetDMG          # the name of target DMG file
    global RootApplications   # reserved directory name for applications
    global LatestOSMacPorts   # True if 'LatestOS with MacPorts'  and targeting LW-*
    global LatestOSHomebrew   # True if 'LatestOS with Homebrew'  and targeting LW-*
    global LatestOSAnaconda3  # True if 'LatestOS with Anaconda3' and targeting LW-*
    global LatestOSHomebrewH  # True if 'LatestOS with Homebrew'  and targeting HW-*
    global DicStdLightHeavyW  # dictionary for LW-* and HW-* packages
    global Item3AppleScript   # ITEM_3 in the Apple script
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
    Usage += "<< Usage of 'makeDMG4mac.py' >>\n"
    Usage += "       for making a DMG file of KLayout 0.28.17 or later on different Apple macOS platforms.\n"
    Usage += "\n"
    Usage += "$ [python] ./makeDMG4mac.py\n"
    Usage += "   option & argument    : descriptions                                               | default value\n"
    Usage += "   ----------------------------------------------------------------------------------+-----------------\n"
    Usage += "   <-p|--pkg <dir>>     : package directory created by `build4mac.py` with [-y|-Y]   | ``\n"
    Usage += "                        : like 'LW-qt5MP.pkg.macos-Monterey-release-Rmp33Pmp311'     | \n"
    Usage += "   <-c|--clean>         : clean the work directory                                   | disabled\n"
    Usage += "   <-m|--make>          : make a compressed DMG file                                 | disabled\n"
    Usage += "                        :   <-c|--clean> and <-m|--make> are mutually exclusive      | \n"
    Usage += "   [-b|--bundle <name>] : forcibly use this bundle name in the DMG                   | ''\n"
    Usage += "   [-s|--serial <num>]  : DMG serial number                                          | 1\n"
    Usage += "   [-u|--unsafe]        : Ignores a few checks (use with caution)                    | disabled\n"
    Usage += "   [-t|--targetdmg]     : Specify output .dmg filename                               | chosen by script\n"
    Usage += "   [-?|--?]             : print this usage and exit                                  | disabled\n"
    Usage += "-------------------------------------------------------------------------------------+-------------------\n"

    ProjectDir = os.getcwd()
    (System, Node, Release, Version, Machine, Processor) = platform.uname()

    if not System == "Darwin":
        print("")
        print( "!!! Sorry. Your system <%s> looks like non-Mac" % System, file=sys.stderr )
        print(Usage)
        quit()

    release = int( Release.split(".")[0] ) # take the first of ['21', '0', '0']
    LatestOS = ""
    if release == 23:
        GenOSName = "macOS"
        Platform  = "Sonoma"
        LatestOS  = Platform
    elif release == 22:
        GenOSName = "macOS"
        Platform  = "Ventura"
        LatestOS  = Platform
    elif release == 21:
        GenOSName = "macOS"
        Platform  = "Monterey"
        LatestOS  = Platform
    else:
        Platform = ""
        print("")
        print( "!!! Sorry. Unsupported major OS release <%d>" % release, file=sys.stderr )
        print(Usage)
        sys.exit(1)

    if not Machine == "x86_64":
        if Machine == "arm64" and Platform in ["Sonoma", "Ventura", "Monterey"]: # with an Apple Silicon Chip
            print("")
            print( "### Your Mac equips an Apple Silicon Chip ###" )
            print("")
        else:
            print("")
            print( "!!! Sorry. Only x86_64/arm64 architecture machine is supported but found <%s>" % Machine, file=sys.stderr )
            print(Usage)
            sys.exit(1)

    PkgDir            = ""
    UnsafePkg         = False
    OpClean           = False
    OpMake            = False
    DefaultBundleName = "klayout.app"
    BundleName        = ""
    DMGSerialNum      = 1
    PackagePrefix     = ""
    QtIdentification  = ""
    RubyPythonID      = ""
    KLVersion         = GetKLayoutVersionFrom( "./version.sh" )
    OccupiedDS        = -1
    BackgroundPNG     = None
    VolumeIcons       = "KLayoutHDD.icns"
    AppleScriptDMG    = "macbuild/Resources/KLayoutDMG.applescript"
    WorkDMG           = "work-KLayout.dmg"
    VolumeDMG         = "KLayout"
    TargetDMG         = ""
    RootApplications  = "/Applications"
    LatestOSMacPorts  = False
    LatestOSHomebrew  = False
    LatestOSAnaconda3 = False
    LatestOSHomebrewH = False
    DicStdLightHeavyW = dict()
    Item3AppleScript  = ""

    # Populate DicStdLightHeavyW
    DicStdLightHeavyW[ "std" ]   = dict() # ST-*
    DicStdLightHeavyW[ "ports" ] = dict() # LW-*
    DicStdLightHeavyW[ "brew" ]  = dict() # LW-*
    DicStdLightHeavyW[ "ana3" ]  = dict() # LW-*
    DicStdLightHeavyW[ "brewH" ] = dict() # HW-*

    DicStdLightHeavyW[ "std" ]["zip"]     = "macbuild/Resources/script-bundle-S.zip"
    DicStdLightHeavyW[ "std" ]["src"]     = "script-bundle-S"
    DicStdLightHeavyW[ "std" ]["des"]     = "MacStdUser-ReadMeFirst"
    DicStdLightHeavyW[ "std" ]["item3"]   = 'set position of item "MacStdUser-ReadMeFirst" to {700, 400}'

    DicStdLightHeavyW[ "ports" ]["zip"]   = "macbuild/Resources/script-bundle-P.zip"
    DicStdLightHeavyW[ "ports" ]["src"]   = "script-bundle-P"
    DicStdLightHeavyW[ "ports" ]["des"]   = "MacPortsUser-ReadMeFirst"
    DicStdLightHeavyW[ "ports" ]["item3"] = 'set position of item "MacPortsUser-ReadMeFirst" to {700, 400}'

    DicStdLightHeavyW[ "brew" ]["zip"]    = "macbuild/Resources/script-bundle-B.zip"
    DicStdLightHeavyW[ "brew" ]["src"]    = "script-bundle-B"
    DicStdLightHeavyW[ "brew" ]["des"]    = "HomebrewUser-ReadMeFirst"
    DicStdLightHeavyW[ "brew" ]["item3"]  = 'set position of item "HomebrewUser-ReadMeFirst" to {700, 400}'

    DicStdLightHeavyW[ "ana3" ]["zip"]    = "macbuild/Resources/script-bundle-A.zip"
    DicStdLightHeavyW[ "ana3" ]["src"]    = "script-bundle-A"
    DicStdLightHeavyW[ "ana3" ]["des"]    = "Anaconda3User-ReadMeFirst"
    DicStdLightHeavyW[ "ana3" ]["item3"]  = 'set position of item "Anaconda3User-ReadMeFirst" to {700, 400}'

    DicStdLightHeavyW[ "brewH" ]["zip"]   = "macbuild/Resources/script-bundle-H.zip"
    DicStdLightHeavyW[ "brewH" ]["src"]   = "script-bundle-H"
    DicStdLightHeavyW[ "brewH" ]["des"]   = "Homebrew-HUser-ReadMeFirst"
    DicStdLightHeavyW[ "brewH" ]["item3"] = 'set position of item "Homebrew-HUser-ReadMeFirst" to {700, 400}'

#------------------------------------------------------------------------------
## To check the contents of the package directory
#
# The package directory name should look like:
#     * ST-qt5MP.pkg.macos-Monterey-release-RsysPsys
#     * LW-qt5Ana3.pkg.macos-Monterey-release-Rana3Pana3
#     * LW-qt5Brew.pkg.macos-Monterey-release-Rhb33Phb311  --- (1)
#     * LW-qt5MP.pkg.macos-Monterey-release-Rmp33Pmp311
#     * HW-qt5Brew.pkg.macos-Monterey-release-RsysPhb311
#
#     * ST-qt6MP.pkg.macos-Monterey-release-RsysPsys
#     * LW-qt6Ana3.pkg.macos-Monterey-release-Rana3Pana3
#     * LW-qt6Brew.pkg.macos-Monterey-release-Rhb33Phb311
#     * LW-qt6MP.pkg.macos-Monterey-release-Rmp33Pmp311
#     * HW-qt6Brew.pkg.macos-Monterey-release-RsysPhb311
#
# Generated DMG will be, for example,
#     (1) ---> LW-klayout-0.28.17-macOS-Monterey-1-qt5Brew-Rhb33Phb311.dmg
#
# @return on success, positive integer in [MB] that tells approx. occupied disc space;
#         on failure, -1
#------------------------------------------------------------------------------
def CheckPkgDirectory():
    global PkgDir
    global UnsafePkg
    global Platform
    global OpClean
    global OpMake
    global DefaultBundleName
    global BundleName
    global PackagePrefix
    global QtIdentification
    global RubyPythonID
    global BackgroundPNG
    global LatestOSMacPorts
    global LatestOSHomebrew
    global LatestOSAnaconda3
    global LatestOSHomebrewH
    global DicStdLightHeavyW
    global Item3AppleScript

    #-----------------------------------------------------------------------------
    # [1] Check the contents of the package directory
    #-----------------------------------------------------------------------------
    if PkgDir == "":
        print( "! Package directory is not specified", file=sys.stderr )
        print(Usage)
        return -1

    if not os.path.isdir(PkgDir):
        print( "! Specified package directory <%s> does not exist" % PkgDir, file=sys.stderr )
        print( "" )
        return -1

    #-----------------------------------------------------------------------------------------------
    # [2] Identify (Qt, Ruby, Python) from PkgDir
    #     * ST-qt5MP.pkg.macos-Monterey-release-RsysPsys
    #     * LW-qt5Ana3.pkg.macos-Monterey-release-Rana3Pana3
    #     * LW-qt5Brew.pkg.macos-Monterey-release-Rhb33Phb311
    #     * LW-qt5MP.pkg.macos-Monterey-release-Rmp33Pmp311
    #     * HW-qt5Brew.pkg.macos-Monterey-release-RsysPhb311
    #     * EX-qt5MP.pkg.macos-Monterey-release-Rhb33Pmp311
    #
    #     * ST-qt6MP.pkg.macos-Monterey-release-RsysPsys
    #     * LW-qt6Ana3.pkg.macos-Monterey-release-Rana3Pana3
    #     * LW-qt6Brew.pkg.macos-Monterey-release-Rhb33Phb311
    #     * LW-qt6MP.pkg.macos-Monterey-release-Rmp33Pmp311
    #     * HW-qt6Brew.pkg.macos-Monterey-release-RsysPhb311
    #-----------------------------------------------------------------------------------------------
    patQRP = u'(ST|LW|HW|EX)([-])([qt5|qt6][0-9A-Za-z]+)([.]pkg[.])([A-Za-z]+[-][A-Za-z]+[-]release[-])([0-9A-Za-z]+)'
    regQRP = re.compile(patQRP)
    if not regQRP.match(PkgDir):
        print( "! Cannot identify (Qt, Ruby, Python) from the package directory name" )
        if UnsafePkg:
            print( "! Ignoring..." )
        else:
            print( "" )
            return -1
    else:
        pkgdirComponents = regQRP.match(PkgDir).groups()
        PackagePrefix    = pkgdirComponents[0]
        QtIdentification = pkgdirComponents[2]
        RubyPythonID     = pkgdirComponents[5]
        if QtIdentification.find('qt5') == 0:
            BackgroundPNG = "KLayoutDMG-BackQt5.png"
        elif QtIdentification.find('qt6') == 0:
            BackgroundPNG = "KLayoutDMG-BackQt6.png"
        else:
            BackgroundPNG = None
            raise Exception( "! neither qt5 nor qt6" )

        #-----------------------------------------------------------------------------
        # [3] Check if the "LatestOS" with MacPorts / Homebrew / Anaconda3
        #-----------------------------------------------------------------------------
        LatestOSSys        = Platform == LatestOS
        LatestOSSys       &= PackagePrefix == "ST"
        LatestOSSys       &= QtIdentification in [ "qt5MP", "qt6MP" ]
        LatestOSSys       &= RubyPythonID in [ "RsysPsys" ]

        LatestOSMacPorts   = Platform == LatestOS
        LatestOSMacPorts  &= PackagePrefix == "LW"
        LatestOSMacPorts  &= QtIdentification in [ "qt5MP", "qt6MP" ]
        LatestOSMacPorts  &= RubyPythonID in [ "Rmp33Pmp311", "Rmp33Pmp39" ]

        LatestOSHomebrew   = Platform == LatestOS
        LatestOSHomebrew  &= PackagePrefix == "LW"
        LatestOSHomebrew  &= QtIdentification in [ "qt5Brew", "qt6Brew" ]
        LatestOSHomebrew  &= RubyPythonID in [ "Rhb33Phb311", "Rhb33Phb39", "Rhb33Phbauto" ]

        LatestOSAnaconda3  = Platform == LatestOS
        LatestOSAnaconda3 &= PackagePrefix == "LW"
        LatestOSAnaconda3 &= QtIdentification in [ "qt5Ana3" ]
        LatestOSAnaconda3 &= RubyPythonID in [ "Rana3Pana3" ]

        LatestOSHomebrewH  = Platform == LatestOS
        LatestOSHomebrewH &= PackagePrefix == "HW"
        LatestOSHomebrewH &= QtIdentification in [ "qt5Brew", "qt6Brew" ]
        LatestOSHomebrewH &= RubyPythonID in [ "RsysPhb311", "RsysPhb39", "RsysPhbauto" ] # Sys-Homebre hybrid

        if LatestOSSys:
            mydic  = DicStdLightHeavyW["std"]
            srcDir = PkgDir + "/" + mydic["src"]
            desDir = PkgDir + "/" + mydic["des"]
            if OpMake:
                with zipfile.ZipFile( mydic["zip"], 'r' ) as zip_ref:
                    zip_ref.extractall(PkgDir)
                os.rename( srcDir, desDir )
            if OpClean:
                if os.path.isdir(srcDir):
                    shutil.rmtree(srcDir)
                if os.path.isdir(desDir):
                    shutil.rmtree(desDir)
            Item3AppleScript = mydic["item3"]

        if LatestOSMacPorts:
            mydic  = DicStdLightHeavyW["ports"]
            srcDir = PkgDir + "/" + mydic["src"]
            desDir = PkgDir + "/" + mydic["des"]
            if OpMake:
                with zipfile.ZipFile( mydic["zip"], 'r' ) as zip_ref:
                    zip_ref.extractall(PkgDir)
                os.rename( srcDir, desDir )
            if OpClean:
                if os.path.isdir(srcDir):
                    shutil.rmtree(srcDir)
                if os.path.isdir(desDir):
                    shutil.rmtree(desDir)
            Item3AppleScript = mydic["item3"]

        if LatestOSHomebrew:
            mydic  = DicStdLightHeavyW["brew"]
            srcDir = PkgDir + "/" + mydic["src"]
            desDir = PkgDir + "/" + mydic["des"]
            if OpMake:
                with zipfile.ZipFile( mydic["zip"], 'r' ) as zip_ref:
                    zip_ref.extractall(PkgDir)
                os.rename( srcDir, desDir )
            if OpClean:
                if os.path.isdir(srcDir):
                    shutil.rmtree(srcDir)
                if os.path.isdir(desDir):
                    shutil.rmtree(desDir)
            Item3AppleScript = mydic["item3"]

        if LatestOSAnaconda3:
            mydic  = DicStdLightHeavyW["ana3"]
            srcDir = PkgDir + "/" + mydic["src"]
            desDir = PkgDir + "/" + mydic["des"]
            if OpMake:
                with zipfile.ZipFile( mydic["zip"], 'r' ) as zip_ref:
                    zip_ref.extractall(PkgDir)
                os.rename( srcDir, desDir )
            if OpClean:
                if os.path.isdir(srcDir):
                    shutil.rmtree(srcDir)
                if os.path.isdir(desDir):
                    shutil.rmtree(desDir)
            Item3AppleScript = mydic["item3"]

        if LatestOSHomebrewH:
            mydic  = DicStdLightHeavyW["brewH"]
            srcDir = PkgDir + "/" + mydic["src"]
            desDir = PkgDir + "/" + mydic["des"]
            if OpMake:
                with zipfile.ZipFile( mydic["zip"], 'r' ) as zip_ref:
                    zip_ref.extractall(PkgDir)
                os.rename( srcDir, desDir )
            if OpClean:
                if os.path.isdir(srcDir):
                    shutil.rmtree(srcDir)
                if os.path.isdir(desDir):
                    shutil.rmtree(desDir)
            Item3AppleScript = mydic["item3"]

    #------------------------------------------------------
    # [4] Check the presence of the default bundle
    #------------------------------------------------------
    os.chdir(PkgDir)
    if not os.path.isdir( DefaultBundleName ):
        print( "! The package directory <%s> does not hold <%s> bundle" % (PkgDir, DefaultBundleName), file=sys.stderr )
        print( "" )
        os.chdir(ProjectDir)
        return -1

    #------------------------------------------------------
    # [5] Check the occupied disk space
    #------------------------------------------------------
    command = "\du -sm %s" % DefaultBundleName
    sizeApp = int( os.popen(command).read().strip("\n").split("\t")[0] )

    #------------------------------------------------------
    # [6] Change the application bundle name if required
    #------------------------------------------------------
    if OpMake and BundleName != "" and BundleName != DefaultBundleName:
        os.rename( DefaultBundleName, BundleName )
    os.chdir(ProjectDir)
    return sizeApp

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
    global BundleName
    global DMGSerialNum
    global UnsafePkg
    global PackagePrefix
    global QtIdentification
    global RubyPythonID
    global KLVersion
    global OccupiedDS
    global TargetDMG
    global Machine

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

    p.add_option( '-b', '--bundle',
                    dest='bundle_name',
                    help="forcibly use this bundle name in the DMG" )

    p.add_option( '-s', '--serial',
                    dest='dmg_serial',
                    help="DMG serial number" )

    p.add_option( '-t', '--targetdmg',
                    dest='target_dmg',
                    help="output DMG filename" )

    p.add_option( '-z', '--unsafe',
                    action='store_true',
                    dest='unsafe',
                    default=False,
                    help="If set, do not check whether pkg folder is empty" )

    p.add_option( '-?', '--??',
                    action='store_true',
                    dest='checkusage',
                    default=False,
                    help='check usage' )

    p.set_defaults( pkg_dir         = "",
                    operation_clean = False,
                    operation_make  = False,
                    bundle_name     = "",
                    target_dmg      = "",
                    dmg_serial      = "1",
                    unsafe          = False,
                    checkusage      = False )

    #-----------------------------------------------------------
    # [1] Parse the command line options
    #-----------------------------------------------------------
    opt, args = p.parse_args()
    if (opt.checkusage):
        print(Usage)
        quit()

    PkgDir       = opt.pkg_dir
    OpClean      = opt.operation_clean
    OpMake       = opt.operation_make
    DMGSerialNum = int(opt.dmg_serial)
    UnsafePkg    = opt.unsafe

    if not opt.bundle_name == "":
        base, ext  = os.path.splitext( os.path.basename(opt.bundle_name) )
        BundleName = base + ".app"
    else:
        BundleName = DefaultBundleName

    if (OpClean and OpMake) or (not OpClean and not OpMake):
        print( "! Specify <-c|--clean> OR <-m|--make>", file=sys.stderr )
        print(Usage)
        quit()

    #------------------------------------------------------------------------------------
    # [2] Check the PKG directory to set QtIdentification, RubyPythonID, and BundleName
    #------------------------------------------------------------------------------------
    OccupiedDS = CheckPkgDirectory()
    if not 0 < OccupiedDS and not UnsafePkg:
        print( "! Failed to check the PKG directory" )
        print( "" )
        quit()

    if opt.target_dmg != "":
        TargetDMG = opt.target_dmg
    else:
        TargetDMG = "%s-klayout-%s-%s-%s-%d-%s-%s.dmg" \
                        % (PackagePrefix, KLVersion, GenOSName, Platform, DMGSerialNum, QtIdentification, RubyPythonID)
        if Machine == "arm64": # with an Apple Silicon Chip
            TargetDMG = Machine + TargetDMG
    return

#------------------------------------------------------------------------------
## Make the target DMG file
#
# @param[in] msg  message to print
#
# @return True on success; False on failure
#------------------------------------------------------------------------------
def MakeTargetDMGFile(msg=""):
    #-----------------------------------------------------------------------
    # The work DMG is mounted like:
    #   /dev/disk6s1 248Mi 228Mi 20Mi 93% 58449 5027 92% /Volumes/KLayout
    #-----------------------------------------------------------------------
    global MountDir   # the mount directory: eg. /Volumes/KLayout
    global FileSys    # the file system    : eg. /dev/disk6s1

    #-------------------------------------------------------------
    # [1] Print message
    #-------------------------------------------------------------
    if not msg == "":
        print(msg)

    #-------------------------------------------------------------
    # [2] Do the following jobs (0) through (14) sequentially
    #-------------------------------------------------------------

    #--------------------------------------------------------
    # (0) Cleanup ProjectDir/
    #--------------------------------------------------------
    CleanUp()

    #--------------------------------------------------------
    # (1) Read the AppleScript template file and generate
    #     the actual one to execute later
    #--------------------------------------------------------
    os.chdir(ProjectDir)
    print( ">>> (1) Preparing AppleScript to execute later..." )
    tempScr = "macbuild/Resources/template-KLayoutDMG.applescript"
    try:
        fd   = open( tempScr, "r" )
        tmpl = fd.read()
        fd.close()
    except Exception as e:
        print( "        ! Failed to read <%s>" % tempScr, file=sys.stderr )
        return False
    else:
        t = string.Template(tmpl)
        # Figures below were determined by experiments for best fit
        applescript = t.safe_substitute(
                                            ORGX='50', ORGY='100',
                                            WIN_WIDTH='1000', WIN_HEIGHT='540',
                                            FULL_PATH_DS_STORE='/Volumes/%s/.DS_Store' % VolumeDMG,
                                            BACKGROUND_PNG_FILE=BackgroundPNG,
                                            ITEM_1='%s' % BundleName,  X1='900', Y1='165',
                                            ITEM_2='Applications',     X2='900', Y2='345',
                                            ITEM_3=Item3AppleScript,
                                            CHECK_BASH='[ -f " & dotDSStore & " ]; echo $?'
                                        )
    try:
        # print(applescript)
        fd = open( AppleScriptDMG, "w" )
        fd.write(applescript)
        fd.close()
    except Exception as e:
        print( "! Failed to write <%s>" % AppleScriptDMG, file=sys.stderr )
        return False
    else:
        print( "        saved <%s>" % AppleScriptDMG )

    #----------------------------------------------------
    # (2) Create a work disk image under ProjectDir/
    #----------------------------------------------------
    if os.path.exists(WorkDMG):
        os.remove(WorkDMG)
    dmgsize  = OccupiedDS + 20 # approx. occupied size plus 20[MB]
    cmdline  = 'hdiutil create -srcfolder %s -volname %s -fs HFS+ -fsargs "-c c=64,a=16,e=16" '
    cmdline += '-format UDRW -size %dm  %s'
    command  = cmdline % (PkgDir, VolumeDMG, dmgsize, WorkDMG)
    print( ">>> (2) Creating a work DMG file <%s> of <%d> [MB] with the volume name of <%s>..." % (WorkDMG, dmgsize, VolumeDMG) )
    os.system(command)
    MountDir = "/Volumes/%s" % VolumeDMG

    #--------------------------------------------------------
    # (3) Check if the mount point 'MountDir' already exists.
    #     If so, unmount it first.
    #--------------------------------------------------------
    command1 = "hdiutil info | grep %s | grep \"/dev/\" | awk '{print $1}'" % VolumeDMG
    print ( ">>> (3) Checking if the mount point <%s> already exists..." % MountDir)
    FileSys = os.popen(command1).read().strip('\n')
    if os.path.isdir(MountDir) and not FileSys == "":
        command2 = "hdiutil detach %s" % FileSys
        os.system(command2)
        print( "        Mount directory <%s> was detached" % MountDir )
    else:
        print( "        Mount directory <%s> does not exist; nothing to do" % MountDir )

    #--------------------------------------------------------
    # (4) Mount the DMG
    #--------------------------------------------------------
    print( ">>> (4) Mounting <%s> to <%s>" % (WorkDMG, MountDir ) )
    command1 = "hdiutil attach %s -readwrite -noverify -quiet -noautoopen" % WorkDMG
    os.system(command1)

    command2 = "hdiutil info | grep %s | grep \"/dev/\" | awk '{print $1}'" % VolumeDMG
    FileSys  = os.popen(command2).read().strip('\n')
    if FileSys == "":
        print( "! Failed to identify the file system on which <%s> is mounted" % VolumeDMG )
        return False
    else:
        print( "        File System = %s" % FileSys )

    #--------------------------------------------------------
    # (5) Copy the background image
    #--------------------------------------------------------
    print( ">>> (5) Copying the background image..." )
    imageSrc  = "macbuild/Resources/%s" % BackgroundPNG
    imageDest = "%s/.background" % MountDir
    if not os.path.isdir(imageDest):
        os.mkdir(imageDest)
    command = "\cp -p %s %s/%s" % (imageSrc, imageDest, BackgroundPNG)
    os.system(command)

    #--------------------------------------------------------
    # (6) Create a symbolic link to /Applications
    #--------------------------------------------------------
    print( ">>> (6) Creating a symbolic link to /Applications..." )
    command = "\ln -s %s %s/%s" % (RootApplications, MountDir, RootApplications)
    os.system(command)

    #--------------------------------------------------------
    # (7) Run the AppleScript
    #--------------------------------------------------------
    print( ">>> (7) Running the AppleScript..." )
    command = "/usr/bin/osascript %s %s" % (AppleScriptDMG, VolumeDMG)
    process = subprocess.Popen( command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True )
    output, error = process.communicate()
    outputStr = output.decode("utf-8")
    errorStr  = error.decode("utf-8")
    if not outputStr == "":
        print( "        STDOUT: %s" % outputStr )
    if not errorStr == "":
        print( "        STDERR: %s" % errorStr )

    #--------------------------------------------------------
    # (8) Copy the custom volume icon
    #--------------------------------------------------------
    sleep(4)
    print( ">>> (8) Copying the volume icon..." )
    iconsSrc  = "macbuild/Resources/%s" % VolumeIcons
    iconsDest = "%s/.VolumeIcon.icns" % MountDir
    command1  = "\cp -p %s %s" % (iconsSrc, iconsDest)
    command2  = "SetFile -c icnC %s" % iconsDest
    os.system(command1)
    sleep(2)
    os.system(command2)
    sleep(2)

    #--------------------------------------------------------
    # (9) Change the permission
    #--------------------------------------------------------
    print( ">>> (9) Changing permission to 755..." )
    command = "\chmod -Rf 755 %s &> /dev/null" % MountDir
    os.system(command)

    #--------------------------------------------------------
    # (10) Set volume bootability and startup disk options.
    #      The folder will open on mount.
    #--------------------------------------------------------
    print( ">>> (10) Setting volume bootability and startup disk options..." )
    command = "bless --folder %s --openfolder %s" % (MountDir, MountDir)
    os.system(command)
    sleep(2)

    #--------------------------------------------------------
    # (11) Set attributes of files and directories
    #--------------------------------------------------------
    print( ">>> (11) Setting attributes of files and directories..." )
    command = "SetFile -a C %s" % MountDir # Custom icon (allowed on folders)
    os.system(command)
    sleep(2)

    #--------------------------------------------------------
    # (12) Unmount the disk image
    #--------------------------------------------------------
    print( ">>> (12) Unmounting the disk image..." )
    command = "hdiutil detach %s" % FileSys
    os.system(command)

    #--------------------------------------------------------
    # (13) Compress the disk image
    #--------------------------------------------------------
    print( "" )
    print( ">>> (13) Compressing the disk image..." )
    command = "hdiutil convert %s -format UDZO -imagekey zlib-level=9 -o %s" % (WorkDMG, TargetDMG)
    os.system(command)
    os.remove(WorkDMG)
    print( "" )
    print( "        generated compressed target DMG <%s>" % TargetDMG )

    #--------------------------------------------------------
    # (14) Compute MD5 checksum
    #--------------------------------------------------------
    print( "" )
    print( ">>> (14) Computing MD5 checksum..." )
    with open( TargetDMG, "rb" ) as f:
        data = f.read()
        md5  = hashlib.md5(data).hexdigest()
        md5 += " *%s\n" % TargetDMG
    f.close()
    path, ext    = os.path.splitext( os.path.basename(TargetDMG) )
    md5TargetDMG = path + ".dmg.md5"
    with open( md5TargetDMG, "w" ) as f:
        f.write(md5)
    f.close()
    print( "        generated MD5 checksum file <%s>" % md5TargetDMG )
    print( "" )

    #-------------------------------------------------------------------------
    # [3] Rename back the application bundle to the default name if required
    #-------------------------------------------------------------------------
    if BundleName != "" and BundleName != DefaultBundleName:
        dirPresent = "%s/%s" % (PkgDir, BundleName)
        dirDefault = "%s/%s" % (PkgDir, DefaultBundleName)
        os.rename( dirPresent, dirDefault )

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
    # [2] Clean up *.dmg*
    #----------------------------------------------------
    os.chdir(ProjectDir)
    dmgs = glob.glob( "*.dmg*" )
    for item in dmgs:
        print("Removing %s" % item)
        os.system( "rm -Rf -- \"%s\"" % item )

    #----------------------------------------------------
    # [3] Clean up AppleScript if any
    #----------------------------------------------------
    if os.path.exists(AppleScriptDMG) and os.path.isfile(AppleScriptDMG):
        os.remove(AppleScriptDMG)

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
        print( "" )
        ok = MakeTargetDMGFile()
        if not ok:
            print( "  !!! Failed to make the target DMG <%s> ..." % TargetDMG, file=sys.stderr )
            print( "", file=sys.stderr )
        else:
            print( "  ### Done making the target DMG" )
            print( "" )
    else:
        print( "" )
        print( "  ### You are going to clean up <%s> directory" % ProjectDir )
        CleanUp()
        print( "  ### Done cleaning up" )
        print( "" )

#===================================================================================
if __name__ == "__main__":
    Main()

#---------------
# End of file
#---------------
