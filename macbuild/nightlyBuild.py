#!/Applications/anaconda3/bin/python3
# -*- coding: utf-8 -*-

import sys
import os
import shutil
import glob
import platform
import optparse
import subprocess
#------------------------------------------------------------------------------
# In general, avoid setting the first line to '#!/usr/bin/env python3'.
# If so, when this script is invoked in the 'KLayoutNightlyBuild.app' script
# bundle created by Automator, the python3 will be the macOS-bundled python3,
# where pandas is not included by default.
# Therefore, it is better to use one of:
#   1) #!/Applications/anaconda3/bin/python3 (Anaconda3)
#   2) #!/usr/local/bin/python3 (Homebrew needs 'pip3 install pandas')
#   3) #!/opt/local/bin/python3 (MacPorts needs 'sudo pip3 install pandas')
#
# However, if we install 'pandas' and its dependencies to the system Python
# environment, we can also set '#!/usr/bin/env python3'.
#------------------------------------------------------------------------------
import pandas as pd

#------------------------------------------------------------------------------
## To test if the platform is a member of valid platforms
#
# @param[in] platforms     valid platforms
#
# @return matching platform name on success; "" on failure
#------------------------------------------------------------------------------
def Test_My_Platform( platforms=[ 'Monterey', 'Ventura', 'Sonoma', 'Sequoia'] ):
    (System, Node, Release, MacVersion, Machine, Processor) = platform.uname()

    if not System == "Darwin":
        return ""

    release = int( Release.split(".")[0] ) # take the first of ['21', '0', '0']
    if   release == 24:
        Platform = "Sequoia"
    elif release == 23:
        Platform = "Sonoma"
    elif release == 22:
        Platform = "Ventura"
    elif release == 21:
        Platform = "Monterey"
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
    buildTargetDic     = dict()
    buildTargetDic[0]  = 'std'
    buildTargetDic[1]  = 'ports'
    buildTargetDic[2]  = 'brew'
    buildTargetDic[3]  = 'brewHW'
    buildTargetDic[4]  = 'ana3'

    buildTargetDic[5]  = 'brewA'
    buildTargetDic[6]  = 'brewAHW'

    buildTargetDic[12] = 'pbrew'   # use MacPorts' Qt and Homebrew's (Ruby, Python)
    buildTargetDic[13] = 'pbrewHW' # use MacPorts' Qt and Homebrew's Python
    return buildTargetDic

#------------------------------------------------------------------------------
## To get the build option dictionary
#
# @param[in] targetDic  build target dictionary
# @param[in] platform   platform name
#
# @return (dictionary1, dictionary2)-tupple
#          dictionary1: key=(qtVer, mnemonic, bdType), value=build option list
#          dictionary2: key=(qtVer, mnemonic, bdType), value=log file name
#------------------------------------------------------------------------------
def Get_Build_Options( targetDic, platform ):
    buildOp = dict()
    logfile = dict()

    for qtVer in [5, 6]:
        if qtVer == 5:
            qtType = "Qt5"
        elif qtVer == 6:
            qtType = "Qt6"

        for key in targetDic.keys():
            target = targetDic[key]
            if target == "std":
                buildOp[(qtVer, "std", "r")] = [ '-q', '%sMacPorts' % qtType, '-r', 'sys',  '-p', 'sys' ]
                logfile[(qtVer, "std", "r")] = "%sMP.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "RsysPsys")
                buildOp[(qtVer, "std", "d")] = [ '-q', '%sMacPorts' % qtType, '-r', 'sys',  '-p', 'sys', '--debug' ]
                logfile[(qtVer, "std", "d")] = "%sMP.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "RsysPsys")
            elif target == "ports":
                buildOp[(qtVer, "ports", "r")] = [ '-q', '%sMacPorts' % qtType, '-r', 'MP33', '-p', 'MP312' ]
                logfile[(qtVer, "ports", "r")] = "%sMP.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "Rmp33Pmp312")
                buildOp[(qtVer, "ports", "d")] = [ '-q', '%sMacPorts' % qtType, '-r', 'MP33', '-p', 'MP312', '--debug' ]
                logfile[(qtVer, "ports", "d")] = "%sMP.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "Rmp33Pmp312")
            elif target == "brew":
                buildOp[(qtVer, "brew", "r")] = [ '-q', '%sBrew' % qtType, '-r', 'HB34', '-p', 'HB312' ]
                logfile[(qtVer, "brew", "r")] = "%sBrew.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "Rhb34Phb312")
                buildOp[(qtVer, "brew", "d")] = [ '-q', '%sBrew' % qtType, '-r', 'HB34', '-p', 'HB312', '--debug' ]
                logfile[(qtVer, "brew", "d")] = "%sBrew.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "Rhb34Phb312")
            elif target == "brewHW":
                buildOp[(qtVer, "brewHW", "r")] = [ '-q', '%sBrew' % qtType, '-r', 'sys',  '-p', 'HB311' ]
                logfile[(qtVer, "brewHW", "r")] = "%sBrew.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "RsysPhb311")
                buildOp[(qtVer, "brewHW", "d")] = [ '-q', '%sBrew' % qtType, '-r', 'sys',  '-p', 'HB311', '--debug' ]
                logfile[(qtVer, "brewHW", "d")] = "%sBrew.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "RsysPhb311")
            elif target == "ana3":
                buildOp[(qtVer, "ana3", "r")] = [ '-q', '%sAna3' % qtType, '-r', 'Ana3', '-p', 'Ana3' ]
                logfile[(qtVer, "ana3", "r")] = "%sAna3.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "Rana3Pana3")
                buildOp[(qtVer, "ana3", "d")] = [ '-q', '%sAna3' % qtType, '-r', 'Ana3', '-p', 'Ana3', '--debug' ]
                logfile[(qtVer, "ana3", "d")] = "%sAna3.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "Rana3Pana3")
            elif target == "brewA":
                buildOp[(qtVer, "brewA", "r")] = [ '-q', '%sBrew' % qtType, '-r', 'HB34', '-p', 'HBAuto' ]
                logfile[(qtVer, "brewA", "r")] = "%sBrew.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "Rhb34Phbauto")
                buildOp[(qtVer, "brewA", "d")] = [ '-q', '%sBrew' % qtType, '-r', 'HB34', '-p', 'HBAuto', '--debug' ]
                logfile[(qtVer, "brewA", "d")] = "%sBrew.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "Rhb34Phbauto")
            elif target == "brewAHW":
                buildOp[(qtVer, "brewAHW", "r")] = [ '-q', '%sBrew' % qtType, '-r', 'sys',  '-p', 'HBAuto' ]
                logfile[(qtVer, "brewAHW", "r")] = "%sBrew.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "RsysPhbauto")
                buildOp[(qtVer, "brewAHW", "d")] = [ '-q', '%sBrew' % qtType, '-r', 'sys',  '-p', 'HBAuto', '--debug' ]
                logfile[(qtVer, "brewAHW", "d")] = "%sBrew.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "RsysPhbauto")
            elif target == "pbrew":
                buildOp[(qtVer, "pbrew", "r")] = [ '-q', '%sMacPorts' % qtType, '-r', 'HB34', '-p', 'HB312' ]
                logfile[(qtVer, "pbrew", "r")] = "%sMP.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "Rhb34Phb312")
                buildOp[(qtVer, "pbrew", "d")] = [ '-q', '%sMacPorts' % qtType, '-r', 'HB34', '-p', 'HB312', '--debug' ]
                logfile[(qtVer, "pbrew", "d")] = "%sMP.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "Rhb34Phb312")
            elif target == "pbrewHW":
                buildOp[(qtVer, "pbrewHW", "r")] = [ '-q', '%sMacPorts' % qtType, '-r', 'sys',  '-p', 'HB311' ]
                logfile[(qtVer, "pbrewHW", "r")] = "%sMP.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "release", "RsysPhb311")
                buildOp[(qtVer, "pbrewHW", "d")] = [ '-q', '%sMacPorts' % qtType, '-r', 'sys',  '-p', 'HB311', '--debug' ]
                logfile[(qtVer, "pbrewHW", "d")] = "%sMP.build.macos-%s-%s-%s.log" % (qtType.lower(), platform, "debug", "RsysPhb311")

        if WithPymod:
            buildOp[(qtVer, "ports", "r")] = buildOp[(qtVer, "ports", "r")] + ['--buildPymod']
            buildOp[(qtVer, "brew", "r")]  = buildOp[(qtVer, "brew", "r")]
            buildOp[(qtVer, "ana3", "r")]  = buildOp[(qtVer, "ana3", "r")]  + ['--buildPymod']
            buildOp[(qtVer, "pbrew", "r")] = buildOp[(qtVer, "pbrew", "r")]

            buildOp[(qtVer, "ports", "d")] = buildOp[(qtVer, "ports", "d")]
            buildOp[(qtVer, "brew", "d")]  = buildOp[(qtVer, "brew", "d")]
            buildOp[(qtVer, "ana3", "d")]  = buildOp[(qtVer, "ana3", "d")]
            buildOp[(qtVer, "pbrew", "d")] = buildOp[(qtVer, "pbrew", "d")]

    return (buildOp, logfile)

#------------------------------------------------------------------------------
## To get the ".macQAT" dictionary for QA Test
#
# @param[in] targetDic  build target dictionary
# @param[in] platform   platform name
#
# @return a dictionary; key=(qtVer, mnemonic, bdType), value=".macQAT" directory
#------------------------------------------------------------------------------
def Get_QAT_Directory( targetDic, platform ):
    dirQAT = dict()

    for qtVer in [5, 6]:
        if qtVer == 5:
            qtType = "Qt5"
        elif qtVer == 6:
            qtType = "Qt6"

        for key in targetDic.keys():
            target = targetDic[key]
            if target == "std":
                dirQAT[(qtVer, "std", "r")] = '%sMP.build.macos-%s-release-RsysPsys.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "std", "d")] = '%sMP.build.macos-%s-debug-RsysPsys.macQAT'   % (qtType.lower(), platform)
            elif target == "ports":
                dirQAT[(qtVer, "ports", "r")] = '%sMP.build.macos-%s-release-Rmp33Pmp312.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "ports", "d")] = '%sMP.build.macos-%s-debug-Rmp33Pmp312.macQAT'   % (qtType.lower(), platform)
            elif target == "brew":
                dirQAT[(qtVer, "brew", "r")] = '%sBrew.build.macos-%s-release-Rhb34Phb312.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "brew", "d")] = '%sBrew.build.macos-%s-debug-Rhb34Phb312.macQAT'   % (qtType.lower(), platform)
            elif target == "brewHW":
                dirQAT[(qtVer, "brewHW", "r")] = '%sBrew.build.macos-%s-release-RsysPhb311.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "brewHW", "d")] = '%sBrew.build.macos-%s-debug-RsysPhb311.macQAT'   % (qtType.lower(), platform)
            elif target == "ana3":
                dirQAT[(qtVer, "ana3", "r")] = '%sAna3.build.macos-%s-release-Rana3Pana3.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "ana3", "d")] = '%sAna3.build.macos-%s-debug-Rana3Pana3.macQAT'   % (qtType.lower(), platform)
            elif target == "brewA":
                dirQAT[(qtVer, "brewA", "r")] = '%sBrew.build.macos-%s-release-Rhb34Phbauto.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "brewA", "d")] = '%sBrew.build.macos-%s-debug-Rhb34Phbauto.macQAT'   % (qtType.lower(), platform)
            elif target == "brewAHW":
                dirQAT[(qtVer, "brewAHW", "r")] = '%sBrew.build.macos-%s-release-RsysPhbauto.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "brewAHW", "d")] = '%sBrew.build.macos-%s-debug-RsysPhbauto.macQAT'   % (qtType.lower(), platform)
            elif target == "pbrew":
                dirQAT[(qtVer, "pbrew", "r")] = '%sMP.build.macos-%s-release-Rhb34Phb312.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "pbrew", "d")] = '%sMP.build.macos-%s-debug-Rhb34Phb312.macQAT'   % (qtType.lower(), platform)
            elif target == "pbrewHW":
                dirQAT[(qtVer, "pbrewHW", "r")] = '%sMP.build.macos-%s-release-RsysPhb311.macQAT' % (qtType.lower(), platform)
                dirQAT[(qtVer, "pbrewHW", "d")] = '%sMP.build.macos-%s-debug-RsysPhb311.macQAT'   % (qtType.lower(), platform)

    return dirQAT

#------------------------------------------------------------------------------
## To get the build option dictionary for making/cleaning DMG
#
# @param[in] targetDic  build target dictionary
# @param[in] platform   platform name
# @param[in] srlDMG     serial number of DMG
# @param[in] makeflag   True to make; False to clean
#
# @return a dictionary; key=(qtVer, mnemonic, bdType), value=build option list
#------------------------------------------------------------------------------
def Get_Package_Options( targetDic, platform, srlDMG, makeflag ):
    packOp = dict()

    if makeflag:
        flag = '-m'
    else:
        flag = '-c'

    for qtVer in [5, 6]:
        if qtVer == 5:
            qtType = "Qt5"
        elif qtVer == 6:
            qtType = "Qt6"

        for key in targetDic.keys():
            target = targetDic[key]
            if target == "std":
                packOp[(qtVer, "std", "r")] = [ '-p', 'ST-%sMP.pkg.macos-%s-release-RsysPsys' % (qtType.lower(), platform),
                                                '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "std", "d")] = [ '-p', 'ST-%sMP.pkg.macos-%s-debug-RsysPsys'   % (qtType.lower(), platform),
                                                '-s', '%d' % srlDMG, '%s' % flag ]
            elif target == "ports":
                packOp[(qtVer, "ports", "r")] = [ '-p', 'LW-%sMP.pkg.macos-%s-release-Rmp33Pmp312' % (qtType.lower(), platform),
                                                  '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "ports", "d")] = [ '-p', 'LW-%sMP.pkg.macos-%s-debug-Rmp33Pmp312'   % (qtType.lower(), platform),
                                                  '-s', '%d' % srlDMG, '%s' % flag ]
            elif target == "brew":
                packOp[(qtVer, "brew", "r")] = [ '-p', 'LW-%sBrew.pkg.macos-%s-release-Rhb34Phb312' % (qtType.lower(), platform),
                                                 '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "brew", "d")] = [ '-p', 'LW-%sBrew.pkg.macos-%s-debug-Rhb34Phb312'   % (qtType.lower(), platform),
                                                 '-s', '%d' % srlDMG, '%s' % flag ]
            elif target == "brewHW":
                packOp[(qtVer, "brewHW", "r")] = [ '-p', 'HW-%sBrew.pkg.macos-%s-release-RsysPhb311' % (qtType.lower(), platform),
                                                   '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "brewHW", "d")] = [ '-p', 'HW-%sBrew.pkg.macos-%s-debug-RsysPhb311'   % (qtType.lower(), platform),
                                                   '-s', '%d' % srlDMG, '%s' % flag ]
            elif target == "ana3":
                packOp[(qtVer, "ana3", "r")] = [ '-p', 'LW-%sAna3.pkg.macos-%s-release-Rana3Pana3' % (qtType.lower(), platform),
                                                 '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "ana3", "d")] = [ '-p', 'LW-%sAna3.pkg.macos-%s-debug-Rana3Pana3'   % (qtType.lower(), platform),
                                                 '-s', '%d' % srlDMG, '%s' % flag ]
            elif target == "brewA":
                packOp[(qtVer, "brewA", "r")] = [ '-p', 'LW-%sBrew.pkg.macos-%s-release-Rhb34Phbauto' % (qtType.lower(), platform),
                                                  '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "brewA", "d")] = [ '-p', 'LW-%sBrew.pkg.macos-%s-debug-Rhb34Phbauto'   % (qtType.lower(), platform),
                                                  '-s', '%d' % srlDMG, '%s' % flag ]
            elif target == "brewAHW":
                packOp[(qtVer, "brewAHW", "r")] = [ '-p', 'HW-%sBrew.pkg.macos-%s-release-RsysPhbauto' % (qtType.lower(), platform),
                                                    '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "brewAHW", "d")] = [ '-p', 'HW-%sBrew.pkg.macos-%s-debug-RsysPhbauto'   % (qtType.lower(), platform),
                                                    '-s', '%d' % srlDMG, '%s' % flag ]
            elif target == "pbrew":
                packOp[(qtVer, "pbrew", "r")] = [ '-p', 'LW-%sMP.pkg.macos-%s-release-Rhb34Phb312' % (qtType.lower(), platform),
                                                  '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "pbrew", "d")] = [ '-p', 'LW-%sMP.pkg.macos-%s-debug-Rhb34Phb312'   % (qtType.lower(), platform),
                                                  '-s', '%d' % srlDMG, '%s' % flag ]
            elif target == "pbrewHW":
                packOp[(qtVer, "pbrewHW", "r")] = [ '-p', 'HW-%sMP.pkg.macos-%s-release-RsysPhb311' % (qtType.lower(), platform),
                                                    '-s', '%d' % srlDMG, '%s' % flag ]
                packOp[(qtVer, "pbrewHW", "d")] = [ '-p', 'HW-%sMP.pkg.macos-%s-debug-RsysPhb311'   % (qtType.lower(), platform),
                                                    '-s', '%d' % srlDMG, '%s' % flag ]
    return packOp

#------------------------------------------------------------------------------
## To parse the command line arguments
#------------------------------------------------------------------------------
def Parse_CommandLine_Arguments():
    global Usage        # usage
    global QtType       # Qt type
    global Target       # target list
    global QtTarget     # list of (Qt, target, bdType)-tuple
    global Build        # operation flag
    global WithPymod    # operation flag
    global QATest       # operation flag
    global QACheck      # operation flag
    global MakeDMG      # operation flag
    global CleanDMG     # operation flag
    global Upload       # operation flag
    global SrlDMG       # DMG serial number
    global Dropbox      # Dropbox directory
    global DryRun       # True for dry-run

    platform = Test_My_Platform()
    if platform in [ "Sequoia", "Sonoma", "Ventura", "Monterey" ]:
        targetopt = "0,1,2,13,4"
    else:
        targetopt = ""

    Usage  = "\n"
    Usage += "----------------------------------------------------------------------------------------------------------\n"
    Usage += " nightlyBuild.py [EXPERIMENTAL]\n"
    Usage += "   << To execute the jobs for making KLayout's DMGs for\n"
    Usage += "                                               macOS Monterey, Ventura, Sonoma, or Sequoia >>\n"
    Usage += "\n"
    Usage += "$ [python] nightlyBuild.py\n"
    Usage += "   option & argument : comment on option if any                              | default value\n"
    Usage += "   --------------------------------------------------------------------------+--------------\n"
    Usage += "   [--qt <type>] : 5='qt5', 6='qt6' (migration to Qt6 is ongoing)            | 5\n"
    Usage += "   [--target <list>] : 0='std', 1='ports', 2='brew', 3='brewHW', 4='ana3',   | '%s'\n" % targetopt
    Usage += "                       5='brewA', 6='brewAHW', 12='pbrew', 13='pbrewHW'      |\n"
    Usage += "                       * with --qt=6, use --target='0,1,2,3' (4 is ignored)  |\n"
    Usage += "   [--qttarget <tuple list>] : ex. '5,1,r'| qt=5, target=1, bdType='r'       | disabled\n"
    Usage += "                                   '5,4,d', '6,4,r', and '6,4,d' are omitted |\n"
    Usage += "      + This option supersedes, if used, the --qt and --target combination.  |\n"
    Usage += "      + You can use this option multiple times.                              |\n"
    Usage += "      + Or you can pass those list by the 'nightlyBuild.csv' file.           |\n"
    Usage += "        A sample file 'macbuild/nightlyBuild.sample.csv' is available.       |\n"
    Usage += "   [--build] : build and deploy                                              | disabled\n"
    Usage += "   [--pymod] : build and deploy Pymod, too (release build only)              | disabled\n"
    Usage += "   [--test]  : run the QA Test                                               | disabled\n"
    Usage += "   [--check] : check the QA Test results                                     | disabled\n"
    Usage += "   [--makedmg|--cleandmg <srlno>] : make or clean DMGs                       | disabled\n"
    Usage += "   [--upload <dropbox>] : upload DMGs to $HOME/Dropbox/klayout/<dropbox>     | disabled\n"
    Usage += "   [--dryrun]           : dry-run for --build option                         | disabled\n"
    Usage += "   [-?|--?]             : print this usage and exit                          | disabled\n"
    Usage += "                                                                             |\n"
    Usage += "      To use this script, make a symbolic link in the project root by:       |\n"
    Usage += "          $ ln -s ./macbuild/nightlyBuild.py .                               |\n"
    Usage += "            + edit and save ./macbuild/nightlyBuild.csv (optional)           |\n"
    Usage += "                                                                             |\n"
    Usage += "      Regular sequence for using this script:                                |\n"
    Usage += "          (1) $ ./nightlyBuild.py  --build  --pymod                          |\n"
    Usage += "          (2)   (confirm the build results)                                  |\n"
    Usage += "          (3) $ ./nightlyBuild.py  --test                                    |\n"
    Usage += "          (4) $ ./nightlyBuild.py  --check (confirm the QA Test results)     |\n"
    Usage += "          (5) $ ./nightlyBuild.py  --makedmg  1                              |\n"
    Usage += "          (6) $ ./nightlyBuild.py  --upload  '0.29.7'                        |\n"
    Usage += "          (7) $ ./nightlyBuild.py  --cleandmg 1                              |\n"
    Usage += "-----------------------------------------------------------------------------+----------------------------\n"

    p = optparse.OptionParser( usage=Usage )
    p.add_option( '--qt',
                    dest='qt_type',
                    help='Qt5 or Qt6 (5)' )

    p.add_option( '--target',
                    dest='targets',
                    help='build target list' )

    p.add_option( '--qttarget',
                    action='append',
                    dest='qt_target',
                    help='(Qt, target, bdType)-tuple' )

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
                    qt_target  = list(),
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

    myPlatform = Test_My_Platform( [ 'Monterey', 'Ventura', 'Sonoma', 'Sequoia' ] )
    if myPlatform == "":
        print( "! Current platform is not [ 'Monterey', 'Ventura', 'Sonoma', 'Sequoia' ]" )
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
        if idx in [0,1,2,3,4,5,6,12,13]:
            Target.append( targetDic[idx] )

    # Populate QtTarget
    QtTarget = list()
    for target in Target:
        QtTarget.append( (QtType, target, 'r') )
    QtType = None
    Target = None
    print( "# The --qt and --target combination specifies for 'r'elease build..." )
    print(QtTarget)

    if len(opt.qt_target) == 1 and opt.qt_target[0] == "nightlyBuild.csv": # reserved file name
        QtTarget     = list()
        withqttarget = True
        df = pd.read_csv( opt.qt_target[0], comment="#" )
        if len(df) == 0:
            print( "! --qttarget==nightlyBuild.csv is used but DataFrame is empty" )
            print(Usage)
            quit()
        for i in range(0, len(df)):
            qt     = df.iloc[i,0]
            idx    = df.iloc[i,1]
            bdType = df.iloc[i,2].lower()[0]
            if (qt == 5 and idx in [0,1,2,3,4,5,6,12,13] and bdType in ['r']) or \
               (qt == 5 and idx in [0,1,2,3,  5,6,12,13] and bdType in ['d']) or \
               (qt == 6 and idx in [0,1,2,3,  5,6,12,13] and bdType in ['r', 'd']):
                QtTarget.append( (qt, targetDic[idx], bdType) )
    elif len(opt.qt_target) > 0:
        QtTarget     = list()
        withqttarget = True
        for item in opt.qt_target:
            qt     = int(item.split(",")[0])
            idx    = int(item.split(",")[1])
            bdType =    (item.split(",")[2]).lower()[0]
            if (qt == 5 and idx in [0,1,2,3,4,5,6,12,13] and bdType in ['r']) or \
               (qt == 5 and idx in [0,1,2,3,  5,6,12,13] and bdType in ['d']) or \
               (qt == 6 and idx in [0,1,2,3,  5,6,12,13] and bdType in ['r', 'd']):
                QtTarget.append( (qt, targetDic[idx], bdType) )
    else:
        withqttarget = False

    if withqttarget:
        if len(QtTarget) > 0:
            print( "# The --qttarget option superseded the --qt and --target combination" )
            print(QtTarget)
        else:
            print( "! --qttarget is used but there is no valid (Qt, target, bdTye)-tuple" )
            print(Usage)
            quit()

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
    pyBuilder  = "./build4mac.py"
    myPlatform = Test_My_Platform()
    buildOp, logfile = Get_Build_Options( Get_Build_Target_Dict(), myPlatform )

    for qttype, key, bdType in QtTarget:
        if key == "ana3" and (qttype == 6 or bdType == 'd'): # anaconda3 does not provide Qt6 | debug_lib
            continue

        deplog = logfile[(qttype, key, bdType)].replace( ".log", ".dep.log" )

        command1 = [ pyBuilder ] + buildOp[(qttype, key, bdType)]

        if key in [ "std", "brewHW", "brewAHW", "pbrewHW" ] :
            command2  = "time"
            command2 += " \\\n  %s" % pyBuilder
            for option in buildOp[(qttype, key, bdType)]:
                command2 += " \\\n  %s" % option
            command2 += " \\\n  %s" % '-y'
            command2 += "  2>&1 | tee %s; \\\n" % deplog
            command2 += "test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0
        else:
            command2  = "time"
            command2 += " \\\n  %s" % pyBuilder
            for option in buildOp[(qttype, key, bdType)]:
                command2 += " \\\n  %s" % option
            command2 += " \\\n  %s" % '-Y'
            command2 += "  2>&1 | tee %s; \\\n" % deplog
            command2 += "test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

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

        if subprocess.call( command2, shell=True ) != 0:
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
# @param[in] excludeList    list of tests to exclude such as ['pymod', 'pya']
#------------------------------------------------------------------------------
def Run_QATest( excludeList ):
    pyRunnerQAT = "./macQAT.py"
    myPlatform  = Test_My_Platform()
    dirQAT      = Get_QAT_Directory( Get_Build_Target_Dict(), myPlatform )

    for qttype, key, bdType in QtTarget:
        if key == "ana3" and (qttype == 6 or bdType == 'd'): # anaconda3 does not provide Qt6 | debug_lib
            continue

        if key == "ana3":
            excludeList += ['pymod']
        exclude = ",".join( sorted( set(excludeList) ) )

        command1 = [ pyRunnerQAT ] + [ '--run' ]
        if not exclude == "":
            command1 += [ '--exclude', '%s' % exclude ]
        print( dirQAT[(qttype, key, bdType)], command1 )
        #continue
        os.chdir( dirQAT[(qttype, key, bdType)] )

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

    for qttype, key, bdType in QtTarget:
        if key == "ana3" and (qttype == 6 or bdType == 'd'): # anaconda3 does not provide Qt6 | debug_lib
            continue

        os.chdir( dirQAT[(qttype, key, bdType)] )
        logfile  = glob.glob( "*.log" )
        command1 = [ tailCommand ] + [ '-n', '%d' % lines ] + logfile
        print( dirQAT[(qttype, key, bdType)], command1 )
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

    for qttype, key, bdType in QtTarget:
        if key == "ana3" and (qttype == 6 or bdType == 'd'): # anaconda3 does not provide Qt6 | debug_lib
            continue

        command1 = [ pyDMGmaker ] + packOp[(qttype, key, bdType)]
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

    for qttype, key, bdType in QtTarget:
        if key == "ana3" and (qttype == 6 or bdType == 'd'): # anaconda3 does not provide Qt6 | debug_lib
            continue

        command1 = [ pyDMGmaker ] + packOp[(qttype, key, bdType)]
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
        Run_QATest( [] ) # ex. ['pymod', 'pya']
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
