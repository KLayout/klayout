#! /usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function  # to use print() of Python 3 in Python >= 2.7
import sys
import os
import shutil
import glob
import optparse
import subprocess

#------------------------------------------------------------------------------
## To populate the build target dictionary
#
# @return a dictionary; key=integer, value=mnemonic
#------------------------------------------------------------------------------
def Get_Build_Target_Dict():
    buildTargetDic    = dict()
    buildTargetDic[0] = 'std'
    buildTargetDic[1] = 'ports'
    buildTargetDic[2] = 'brew'
    buildTargetDic[3] = 'brewHW'
    buildTargetDic[4] = 'ana3'

    buildTargetDic[5] = 'brewA'
    buildTargetDic[6] = 'brewAHW'
    return buildTargetDic

#------------------------------------------------------------------------------
## To get the build option dictionary
#
# @param[in] targetDic  build target dictionary
#
# @return a dictionary; key=mnemonic, value=build option list
#------------------------------------------------------------------------------
def Get_Build_Options( targetDic ):
    buildOp = dict()
    for key in targetDic.keys():
        target = targetDic[key]
        if target == "std": # use 'Qt5Brew' that provides Qt 5.15.1 to run on "Big Sur", too
            buildOp["std"]     = [ '-q', 'Qt5Brew',     '-r', 'sys',  '-p', 'sys'    ]
        elif target == "ports":
            buildOp["ports"]   = [ '-q', 'Qt5MacPorts', '-r', 'MP27', '-p', 'MP38'   ]
        elif target == "brew":
            buildOp["brew"]    = [ '-q', 'Qt5Brew',     '-r', 'HB27', '-p', 'HB38'   ]
        elif target == "brewHW":
            buildOp["brewHW"]  = [ '-q', 'Qt5Brew',     '-r', 'sys',  '-p', 'HB38'   ]
        elif target == "ana3":
            buildOp["ana3"]    = [ '-q', 'Qt5Ana3',     '-r', 'Ana3', '-p', 'Ana3'   ]
        elif target == "brewA":
            buildOp["brewA"]   = [ '-q', 'Qt5Brew',     '-r', 'HB27', '-p', 'HBAuto' ]
        elif target == "brewAHW":
            buildOp["brewAHW"] = [ '-q', 'Qt5Brew',     '-r', 'sys',  '-p', 'HBAuto' ]
    return buildOp

#------------------------------------------------------------------------------
## To get the ".macQAT" dictionary for QA Test
#
# @param[in] targetDic  build target dictionary
#
# @return a dictionary; key=mnemonic, value=".macQAT" directory
#------------------------------------------------------------------------------
def Get_QAT_Directory( targetDic ):
    dirQAT = dict()
    for key in targetDic.keys():
        target = targetDic[key]
        if target == "std":
            dirQAT["std"]       = 'qt5Brew.build.macos-Catalina-release-RsysPsys.macQAT'
        elif target == "ports":
            dirQAT["ports"]     = 'qt5MP.build.macos-Catalina-release-Rmp27Pmp38.macQAT'
        elif target == "brew":
            dirQAT["brew"]      = 'qt5Brew.build.macos-Catalina-release-Rhb27Phb38.macQAT'
        elif target == "brewHW":
            dirQAT["brewHW"]    = 'qt5Brew.build.macos-Catalina-release-RsysPhb38.macQAT'
        elif target == "ana3":
            dirQAT["ana3"]      = 'qt5Ana3.build.macos-Catalina-release-Rana3Pana3.macQAT'
        elif target == "brewA":
            dirQAT["brewA"]     = 'qt5Brew.build.macos-Catalina-release-Rhb27Phbauto.macQAT'
        elif target == "brewAHW":
            dirQAT["brewAHW"]   = 'qt5Brew.build.macos-Catalina-release-RsysPhbauto.macQAT'
    return dirQAT

#------------------------------------------------------------------------------
## To get the build option dictionary for making/cleaning DMG
#
# @param[in] targetDic  build target dictionary
# @param[in] srlDMG     serial number of DMG
# @param[in] makeflag   True to make; False to clean
#
# @return a dictionary; key=mnemonic, value=build option list
#------------------------------------------------------------------------------
def Get_Package_Options( targetDic, srlDMG, makeflag ):
    if makeflag:
        flag = '-m'
    else:
        flag = '-c'

    packOp = dict()
    for key in targetDic.keys():
        target = targetDic[key]
        if target == "std":
            packOp["std"]       = [ '-p', 'ST-qt5Brew.pkg.macos-Catalina-release-RsysPsys',     '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "ports":
            packOp["ports"]     = [ '-p', 'LW-qt5MP.pkg.macos-Catalina-release-Rmp27Pmp38',     '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "brew":
            packOp["brew"]      = [ '-p', 'LW-qt5Brew.pkg.macos-Catalina-release-Rhb27Phb38',   '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "brewHW":
            packOp["brewHW"]    = [ '-p', 'HW-qt5Brew.pkg.macos-Catalina-release-RsysPhb38',    '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "ana3":
            packOp["ana3"]      = [ '-p', 'LW-qt5Ana3.pkg.macos-Catalina-release-Rana3Pana3',   '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "brewA":
            packOp["brewA"]     = [ '-p', 'LW-qt5Brew.pkg.macos-Catalina-release-Rhb27Phbauto', '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "brewAHW":
            packOp["brewAHW"]   = [ '-p', 'HW-qt5Brew.pkg.macos-Catalina-release-RsysPhbauto',  '-s', '%d' % srlDMG, '%s' % flag ]
    return packOp

#------------------------------------------------------------------------------
## To parse the command line arguments
#------------------------------------------------------------------------------
def Parse_CommandLine_Arguments():
    global Usage     # usage
    global Target    # target list
    global Build     # operation flag
    global QATest    # operation flag
    global QACheck   # operation flag
    global MakeDMG   # operation flag
    global CleanDMG  # operation flag
    global Upload    # operation flag
    global SrlDMG    # DMG serial number
    global Dropbox   # Dropbox directory

    Usage  = "\n"
    Usage += "--------------------------------------------------------------------------------------------\n"
    Usage += " nightyCatalina.py [EXPERIMENTAL] \n"
    Usage += "   << To execute the jobs for making KLatyout's DMGs for macOS Catalina >> \n"
    Usage += "\n"
    Usage += "$ [python] nightyCatalina.py \n"
    Usage += "   option & argument : comment on option if any                            | default value\n"
    Usage += "   ------------------------------------------------------------------------+--------------\n"
    Usage += "   [--target <list>] : 0='std', 1='ports', 2='brew', 3='brewHW', 4='ana3', | '0,1,2,3,4'\n"
    Usage += "                       5='brewA', 6='brewAHW'                              | \n"
    Usage += "   [--build] : build and deploy                                            | disabled\n"
    Usage += "   [--test]  : run the QA Test                                             | disabled\n"
    Usage += "   [--check] : check the QA Test results                                   | disabled\n"
    Usage += "   [--makedmg|--cleandmg <srlno>] : make or clean DMGs                     | disabled\n"
    Usage += "   [--upload <dropbox>] : upload DMGs to $HOME/Dropbox/klayout/<dropbox>   | disabled\n"
    Usage += "   [-?|--?]             : print this usage and exit                        | disabled\n"
    Usage += "                                                                           | \n"
    Usage += "      Standard sequence for using this script:                             | \n"
    Usage += "          (1) $ ./nightyCatalina.py  --build                               | \n"
    Usage += "          (2)   (confirm the build results)                                | \n"
    Usage += "          (3) $ ./nightyCatalina.py  --test                                | \n"
    Usage += "          (4) $ ./nightyCatalina.py  --check (confirm the QA Test results) | \n"
    Usage += "          (5) $ ./nightyCatalina.py  --makedmg  1                          | \n"
    Usage += "          (6) $ ./nightyCatalina.py  --upload  '0.26.9'                    | \n"
    Usage += "          (7) $ ./nightyCatalina.py  --cleandmg 1                          | \n"
    Usage += "---------------------------------------------------------------------------+----------------\n"

    p = optparse.OptionParser( usage=Usage )
    p.add_option( '--target',
                    dest='targets',
                    help='build target list' )

    p.add_option( '--build',
                    action='store_true',
                    dest='build',
                    default=False,
                    help='build and deploy' )

    p.add_option( '--test',
                    action='store_true',
                    dest='qa_test',
                    default=False,
                    help='run the QA Test' )

    p.add_option( '--check',
                    action='store_true',
                    dest='qa_check',
                    default=False,
                    help='check the QA Test results' )

    p.add_option( '--makedmg',
                    dest='makedmg',
                    help='make DMG' )

    p.add_option( '--cleandmg',
                    dest='cleandmg',
                    help='clean DMG' )

    p.add_option( '--upload',
                    dest='upload',
                    help='upload to Dropbox' )

    p.add_option( '-?', '--??',
                    action='store_true',
                    dest='checkusage',
                    default=False,
                    help='check usage' )

    p.set_defaults( targets    = "0,1,2,3,4",
                    build      = False,
                    qa_test    = False,
                    qa_check   = False,
                    makedmg    = "",
                    cleandmg   = "",
                    upload     = "",
                    checkusage = False )

    opt, args = p.parse_args()
    if opt.checkusage:
        print(Usage)
        quit()

    targetDic = Get_Build_Target_Dict()
    Target    = list()
    for idx in sorted( list( set( [ int(item) for item in opt.targets.split(",") ] ) ) ):
        if idx in range(0, 7):
            Target.append( targetDic[idx] )

    Build    = opt.build
    QATest   = opt.qa_test
    QACheck  = opt.qa_check
    MakeDMG  = False
    CleanDMG = False
    Upload   = False

    if not opt.makedmg == "":
        MakeDMG = True
        SrlDMG  = int(opt.makedmg)

    if not opt.cleandmg == "":
        CleanDMG = True
        SrlDMG   = int(opt.cleandmg)

    if MakeDMG and CleanDMG:
        print( "! --makedmg and --cleandmg cannot be used simultaneously" )
        print(Usage)
        quit()

    if not opt.upload == "":
        Upload  = True
        Dropbox = opt.upload

    if not (Build or QATest or QACheck or MakeDMG or CleanDMG or Upload):
        print( "! No option selected" )
        print(Usage)
        quit()

#------------------------------------------------------------------------------
## To build and deploy
#------------------------------------------------------------------------------
def Build_Deploy():
    pyBuilder = "./build4mac.py"
    buildOp   = Get_Build_Options( Get_Build_Target_Dict() )

    for key in Target:
        command1 = [ pyBuilder ] + buildOp[key]
        if key in [ "std", "brewHW", "brewAHW" ] :
            command2 = [ pyBuilder ] + buildOp[key] + ['-y']
        else:
            command2 = [ pyBuilder ] + buildOp[key] + ['-Y']
        print(command1)
        print(command2)
        #continue

        if subprocess.call( command1, shell=False ) != 0:
            print( "", file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to build KLayout" % pyBuilder, file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            sys.exit(1)
        else:
            print( "", file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "### <%s>: successfully built KLayout" % pyBuilder, file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "", file=sys.stderr )

        if subprocess.call( command2, shell=False ) != 0:
            print( "", file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to deploy KLayout" % pyBuilder, file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            sys.exit(1)
        else:
            print( "", file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "### <%s>: successfully deployed KLayout" % pyBuilder, file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "", file=sys.stderr )

#------------------------------------------------------------------------------
## To run the QA tests
#
# @param[in] exclude     test to exclude such as 'pymod,pya'
#------------------------------------------------------------------------------
def Run_QATest( exclude ):
    pyRunnerQAT = "./macQAT.py"
    dirQAT      = Get_QAT_Directory( Get_Build_Target_Dict() )

    for key in Target:
        command1 = [ pyRunnerQAT ] + [ '--run', '--exclude', '%s' % exclude ]
        print( dirQAT[key], command1 )
        #continue
        os.chdir( dirQAT[key] )

        if subprocess.call( command1, shell=False ) != 0:
            print( "", file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to run the QA Test" % pyRunnerQAT, file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            sys.exit(1)
        else:
            print( "", file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "### <%s>: successfully ran the QA Test" % pyRunnerQAT, file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "", file=sys.stderr )

        os.chdir( "../" )

#------------------------------------------------------------------------------
## To check the QA test results
#
# @param[in] lines  number of lines to dump from the tail
#------------------------------------------------------------------------------
def Check_QATest_Results( lines ):
    tailCommand = "/usr/bin/tail"
    dirQAT      = Get_QAT_Directory( Get_Build_Target_Dict() )

    for key in Target:
        os.chdir( dirQAT[key] )
        logfile  = glob.glob( "*.log" )
        command1 = [ tailCommand ] + [ '-n', '%d' % lines ] + logfile
        print( dirQAT[key], command1 )
        #continue

        if subprocess.call( command1, shell=False ) != 0:
            print( "", file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to check the QA Test results" % tailCommand, file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            sys.exit(1)
        else:
            print( "", file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "### <%s>: successfully checked the QA Test results" % tailCommand, file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "", file=sys.stderr )

        os.chdir( "../" )

#------------------------------------------------------------------------------
## To make DMGs
#
# @param[in] srlDMG     DMG's serial number
#------------------------------------------------------------------------------
def DMG_Make( srlDMG ):
    pyDMGmaker = "./makeDMG4mac.py"
    stashDMG   = "./DMGStash"
    packOp     = Get_Package_Options( Get_Build_Target_Dict(), srlDMG, makeflag=True )

    if os.path.isdir( stashDMG ):
        shutil.rmtree( stashDMG )
    os.mkdir( stashDMG )

    for key in Target:
        command1 = [ pyDMGmaker ] + packOp[key]
        print(command1)
        #continue

        if subprocess.call( command1, shell=False ) != 0:
            print( "", file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to make KLayout DMG" % pyDMGmaker, file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            sys.exit(1)
        else:
            print( "", file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "### <%s>: successfully made KLayout DMG" % pyDMGmaker, file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "", file=sys.stderr )

        dmgs = glob.glob( "*.dmg*" )
        for item in dmgs:
            shutil.move( item, stashDMG )

#------------------------------------------------------------------------------
## To clean up DMGs
#
# @param[in] srlDMG     DMG's serial number
#------------------------------------------------------------------------------
def DMG_Clean( srlDMG ):
    pyDMGmaker = "./makeDMG4mac.py"
    stashDMG   = "./DMGStash"
    packOp     = Get_Package_Options( Get_Build_Target_Dict(), srlDMG, makeflag=False )

    if os.path.isdir( stashDMG ):
        shutil.rmtree( stashDMG )

    for key in Target:
        command1 = [ pyDMGmaker ] + packOp[key]
        print(command1)
        #continue

        if subprocess.call( command1, shell=False ) != 0:
            print( "", file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to clean KLayout DMG" % pyDMGmaker, file=sys.stderr )
            print( "-----------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            sys.exit(1)
        else:
            print( "", file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "### <%s>: successfully cleaned KLayout DMG" % pyDMGmaker, file=sys.stderr )
            print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
            print( "", file=sys.stderr )

#------------------------------------------------------------------------------
## To upload DMGs to Dropbox
#
# @param[in] targetdir     existing target directory such as "0.26.9"
#------------------------------------------------------------------------------
def Upload_To_Dropbox( targetdir ):
    stashDMG = "./DMGStash"

    distDir = os.environ["HOME"] + "/Dropbox/klayout/" + targetdir
    if not os.path.isdir(distDir):
        os.makedirs(distDir)

    dmgs = glob.glob( "%s/*.dmg*" % stashDMG )
    for item in dmgs:
        shutil.copy2( item, distDir )

#------------------------------------------------------------------------------
## The main function
#------------------------------------------------------------------------------
def Main():
    Parse_CommandLine_Arguments()

    if Build:
        Build_Deploy()
    if QATest:
        Run_QATest( 'pymod,pya' )
    if QACheck:
        Check_QATest_Results( 20 )
    elif MakeDMG:
        DMG_Make( SrlDMG )
    elif Upload:
        Upload_To_Dropbox( Dropbox )
    elif CleanDMG:
        DMG_Clean( SrlDMG )

#===================================================================================
if __name__ == "__main__":
    Main()

#---------------
# End of file
#---------------
