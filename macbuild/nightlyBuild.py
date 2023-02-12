#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import shutil
import glob
import platform
import optparse
import subprocess

#------------------------------------------------------------------------------
## To test if the platform is a member of valid platforms
#
# @param[in] platforms     valid platforms
#
# @return matching platform name on success; "" on failure
#------------------------------------------------------------------------------
def Test_My_Platform( platforms=[ 'Catalina', 'BigSur', 'Monterey', 'Ventura' ] ):
    (System, Node, Release, MacVersion, Machine, Processor) = platform.uname()

    if not System == "Darwin":
        return ""

    release = int( Release.split(".")[0] ) # take the first of ['19', '0', '0']
    if   release == 22:
        Platform = "Ventura"
    elif release == 21:
        Platform = "Monterey"
    elif release == 20:
        Platform = "BigSur"
    elif release == 19:
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

    if Platform in platforms:
        return Platform
    else:
        return ""

#------------------------------------------------------------------------------
## To populate the build target dictionary
#
# @return a dictionary; key=integer, value=mnemonic
#------------------------------------------------------------------------------
def Get_Build_Target_Dict():
    buildTargetDic    = dict()
    buildTargetDic[0] = 'std'   # has been restored in 0.28.3
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
    if QtType == 5:
        qtType = "Qt5"
    else:
        qtType = "Qt6"

    buildOp = dict()
    for key in targetDic.keys():
        target = targetDic[key]
        if target == "std": # use 'Qt5MacPorts' that provides Qt 5.15.2~ to run on "Big Sur", too
            buildOp["std"]     = [ '-q', '%sMacPorts' % qtType, '-r', 'sys',  '-p', 'sys'    ]
        elif target == "ports":
            buildOp["ports"]   = [ '-q', '%sMacPorts' % qtType, '-r', 'MP32', '-p', 'MP39'   ]
        elif target == "brew":
            buildOp["brew"]    = [ '-q', '%sBrew' % qtType,     '-r', 'HB32', '-p', 'HB39'   ]
        elif target == "brewHW":
            buildOp["brewHW"]  = [ '-q', '%sBrew' % qtType,     '-r', 'sys',  '-p', 'HB39'   ]
        elif target == "ana3":
            buildOp["ana3"]    = [ '-q', '%sAna3' % qtType,     '-r', 'Ana3', '-p', 'Ana3'   ]
        elif target == "brewA":
            buildOp["brewA"]   = [ '-q', '%sBrew' % qtType,     '-r', 'HB32', '-p', 'HBAuto' ]
        elif target == "brewAHW":
            buildOp["brewAHW"] = [ '-q', '%sBrew' % qtType,     '-r', 'sys',  '-p', 'HBAuto' ]

    if WithPymod:
        buildOp["ports"] = buildOp["ports"] + ['--buildPymod']
        buildOp["brew"]  = buildOp["brew"]  + ['--buildPymod']
        buildOp["ana3"]  = buildOp["ana3"]  + ['--buildPymod']
    return buildOp

#------------------------------------------------------------------------------
## To get the ".macQAT" dictionary for QA Test
#
# @param[in] targetDic  build target dictionary
# @param[in] platform   platform name
#
# @return a dictionary; key=mnemonic, value=".macQAT" directory
#------------------------------------------------------------------------------
def Get_QAT_Directory( targetDic, platform ):
    if QtType == 5:
        qtType = "qt5"
    else:
        qtType = "qt6"

    dirQAT = dict()
    for key in targetDic.keys():
        target = targetDic[key]
        if target == "std":
            dirQAT["std"]       = '%sMP.build.macos-%s-release-RsysPsys.macQAT' % (qtType, platform)
        elif target == "ports":
            dirQAT["ports"]     = '%sMP.build.macos-%s-release-Rmp32Pmp39.macQAT' % (qtType, platform)
        elif target == "brew":
            dirQAT["brew"]      = '%sBrew.build.macos-%s-release-Rhb32Phb39.macQAT' % (qtType, platform)
        elif target == "brewHW":
            dirQAT["brewHW"]    = '%sBrew.build.macos-%s-release-RsysPhb39.macQAT' % (qtType, platform)
        elif target == "ana3":
            dirQAT["ana3"]      = '%sAna3.build.macos-%s-release-Rana3Pana3.macQAT' % (qtType, platform)
        elif target == "brewA":
            dirQAT["brewA"]     = '%sBrew.build.macos-%s-release-Rhb32Phbauto.macQAT' % (qtType, platform)
        elif target == "brewAHW":
            dirQAT["brewAHW"]   = '%sBrew.build.macos-%s-release-RsysPhbauto.macQAT' % (qtType, platform)
    return dirQAT

#------------------------------------------------------------------------------
## To get the build option dictionary for making/cleaning DMG
#
# @param[in] targetDic  build target dictionary
# @param[in] platform   platform name
# @param[in] srlDMG     serial number of DMG
# @param[in] makeflag   True to make; False to clean
#
# @return a dictionary; key=mnemonic, value=build option list
#------------------------------------------------------------------------------
def Get_Package_Options( targetDic, platform, srlDMG, makeflag ):
    if QtType == 5:
        qtType = "qt5"
    else:
        qtType = "qt6"

    if makeflag:
        flag = '-m'
    else:
        flag = '-c'

    packOp = dict()
    for key in targetDic.keys():
        target = targetDic[key]
        if target == "std":
            packOp["std"]       = [ '-p', 'ST-%sMP.pkg.macos-%s-release-RsysPsys' % (qtType, platform),
                                    '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "ports":
            packOp["ports"]     = [ '-p', 'LW-%sMP.pkg.macos-%s-release-Rmp32Pmp39' % (qtType, platform),
                                    '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "brew":
            packOp["brew"]      = [ '-p', 'LW-%sBrew.pkg.macos-%s-release-Rhb32Phb39' % (qtType, platform),
                                    '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "brewHW":
            packOp["brewHW"]    = [ '-p', 'HW-%sBrew.pkg.macos-%s-release-RsysPhb39' % (qtType, platform),
                                    '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "ana3":
            packOp["ana3"]      = [ '-p', 'LW-%sAna3.pkg.macos-%s-release-Rana3Pana3' % (qtType, platform),
                                    '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "brewA":
            packOp["brewA"]     = [ '-p', 'LW-%sBrew.pkg.macos-%s-release-Rhb32Phbauto' % (qtType, platform),
                                    '-s', '%d' % srlDMG, '%s' % flag ]
        elif target == "brewAHW":
            packOp["brewAHW"]   = [ '-p', 'HW-%sBrew.pkg.macos-%s-release-RsysPhbauto' % (qtType, platform),
                                    '-s', '%d' % srlDMG, '%s' % flag ]
    return packOp

#------------------------------------------------------------------------------
## To parse the command line arguments
#------------------------------------------------------------------------------
def Parse_CommandLine_Arguments():
    global Usage     # usage
    global QtType    # Qt type
    global Target    # target list
    global Build     # operation flag
    global WithPymod # operation flag
    global QATest    # operation flag
    global QACheck   # operation flag
    global MakeDMG   # operation flag
    global CleanDMG  # operation flag
    global Upload    # operation flag
    global SrlDMG    # DMG serial number
    global Dropbox   # Dropbox directory
    global DryRun    # True for dry-run

    platform = Test_My_Platform()
    if platform in [ "Ventura", "Monterey", "BigSur" ]:
        targetopt = "1,2,3,4"
    else:
        targetopt = "0,1,2,3,4"

    Usage  = "\n"
    Usage += "----------------------------------------------------------------------------------------------------------\n"
    Usage += " nightlyBuild.py [EXPERIMENTAL] \n"
    Usage += "   << To execute the jobs for making KLayout's DMGs for macOS Catalina, Big Sur, Monterey, or Ventura >>\n"
    Usage += "\n"
    Usage += "$ [python] nightlyBuild.py\n"
    Usage += "   option & argument : comment on option if any                            | default value\n"
    Usage += "   ------------------------------------------------------------------------+--------------\n"
    Usage += "   [--qt <type>] : 5='qt5', 6='qt6' (migration to Qt6 is ongoing)          | 5\n"
    Usage += "   [--target <list>] : 0='std' 1='ports', 2='brew', 3='brewHW', 4='ana3',  | '%s'\n" % targetopt
    Usage += "                       5='brewA', 6='brewAHW'                              | \n"
    Usage += "                       * with --qt=6, use --target='2,3' (4 is ignored)    | \n"
    Usage += "   [--build] : build and deploy                                            | disabled\n"
    Usage += "   [--pymod] : build and deploy Pymod, too                                 | disabled\n"
    Usage += "   [--test]  : run the QA Test                                             | disabled\n"
    Usage += "   [--check] : check the QA Test results                                   | disabled\n"
    Usage += "   [--makedmg|--cleandmg <srlno>] : make or clean DMGs                     | disabled\n"
    Usage += "   [--upload <dropbox>] : upload DMGs to $HOME/Dropbox/klayout/<dropbox>   | disabled\n"
    Usage += "   [--dryrun]           : dry-run for --build option                       | disabled\n"
    Usage += "   [-?|--?]             : print this usage and exit                        | disabled\n"
    Usage += "                                                                           | \n"
    Usage += "      To use this script, make a symbolic link in the project root by:     | \n"
    Usage += "          $ ln -s ./macbuild/nightlyBuild.py .                             | \n"
    Usage += "                                                                           | \n"
    Usage += "      Regular sequence for using this script:                              | \n"
    Usage += "          (1) $ ./nightlyBuild.py  --build  --pymod                        | \n"
    Usage += "          (2)   (confirm the build results)                                | \n"
    Usage += "          (3) $ ./nightlyBuild.py  --test                                  | \n"
    Usage += "          (4) $ ./nightlyBuild.py  --check (confirm the QA Test results)   | \n"
    Usage += "          (5) $ ./nightlyBuild.py  --makedmg  1                            | \n"
    Usage += "          (6) $ ./nightlyBuild.py  --upload  '0.28.4'                      | \n"
    Usage += "          (7) $ ./nightlyBuild.py  --cleandmg 1                            | \n"
    Usage += "---------------------------------------------------------------------------+------------------------------\n"

    p = optparse.OptionParser( usage=Usage )
    p.add_option( '--qt',
                    dest='qt_type',
                    help='Qt5 or Qt6 (5)' )

    p.add_option( '--target',
                    dest='targets',
                    help='build target list' )

    p.add_option( '--build',
                    action='store_true',
                    dest='build',
                    default=False,
                    help='build and deploy' )

    p.add_option( '--pymod',
                    action='store_true',
                    dest='with_pymod',
                    default=False,
                    help='build and deploy Pymod, too ' )

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

    p.add_option( '--dryrun',
                    action='store_true',
                    dest='dry_run',
                    default=False,
                    help='dry-run' )

    p.add_option( '-?', '--??',
                    action='store_true',
                    dest='checkusage',
                    default=False,
                    help='check usage' )

    p.set_defaults( qt_type    = "5",
                    targets    = "%s" % targetopt,
                    build      = False,
                    with_pymod = False,
                    qa_test    = False,
                    qa_check   = False,
                    makedmg    = "",
                    cleandmg   = "",
                    upload     = "",
                    dry_run    = False,
                    checkusage = False )

    opt, args = p.parse_args()
    if opt.checkusage:
        print(Usage)
        quit()

    myPlatform = Test_My_Platform( [ 'Catalina', 'BigSur', 'Monterey', 'Ventura' ] )
    if myPlatform == "":
        print( "! Current platform is not [ 'Catalina', 'BigSur', 'Monterey', 'Ventura' ]" )
        print(Usage)
        quit()

    QtType = int(opt.qt_type)
    if not QtType in [5, 6]:
        print( "! Invalid Qt type <%d>" % QtType )
        print(Usage)
        quit()

    targetIdx = list()
    for target in [ int(item) for item in opt.targets.split(",") ]:
        if not target in targetIdx:
            targetIdx.append(target)  # first appeared and non-duplicated index

    targetDic = Get_Build_Target_Dict()
    Target    = list()
    for idx in targetIdx:
        if idx in range(0, 7):  # '0' has been restored in 0.28.3
            Target.append( targetDic[idx] )

    Build     = opt.build
    WithPymod = opt.with_pymod
    QATest    = opt.qa_test
    QACheck   = opt.qa_check
    MakeDMG   = False
    CleanDMG  = False
    Upload    = False
    DryRun    = opt.dry_run

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
        print( "! No action selected" )
        print(Usage)
        quit()

#------------------------------------------------------------------------------
## To build and deploy
#------------------------------------------------------------------------------
def Build_Deploy():
    pyBuilder = "./build4mac.py"
    buildOp   = Get_Build_Options( Get_Build_Target_Dict() )

    for key in Target:
        if key == "ana3" and QtType == 6: # anaconda3 does not provide Qt6 so far
            continue

        command1 = [ pyBuilder ] + buildOp[key]
        if key in [ "std", "brewHW", "brewAHW" ] :
            command2 = [ pyBuilder ] + buildOp[key] + ['-y']
        else:
            command2 = [ pyBuilder ] + buildOp[key] + ['-Y']

        if DryRun:
            print( "### Target = <%s> ###" % key )
            print(command1)
            print(command2)
            print( "" )
            continue

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
    myPlatform  = Test_My_Platform()
    dirQAT      = Get_QAT_Directory( Get_Build_Target_Dict(), myPlatform )

    for key in Target:
        if key == 4 and QtType == 6: # anaconda3 does not provide Qt6 so far
            continue

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
    myPlatform  = Test_My_Platform()
    dirQAT      = Get_QAT_Directory( Get_Build_Target_Dict(), myPlatform )

    for key in Target:
        if key == 4 and QtType == 6: # anaconda3 does not provide Qt6 so far
            continue

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
    myPlatform = Test_My_Platform()
    packOp     = Get_Package_Options( Get_Build_Target_Dict(), myPlatform, srlDMG, makeflag=True )

    if os.path.isdir( stashDMG ):
        shutil.rmtree( stashDMG )
    os.mkdir( stashDMG )

    for key in Target:
        if key == 4 and QtType == 6: # anaconda3 does not provide Qt6 so far
            continue

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
    myPlatform = Test_My_Platform()
    packOp     = Get_Package_Options( Get_Build_Target_Dict(), myPlatform, srlDMG, makeflag=False )

    if os.path.isdir( stashDMG ):
        shutil.rmtree( stashDMG )

    for key in Target:
        if key == 4 and QtType == 6: # anaconda3 does not provide Qt6 so far
            continue

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
