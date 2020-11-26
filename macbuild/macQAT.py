#!/usr/bin/env python
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/macQAT.py"
#
#  The top Python script to run "ut_runner" after building KLayout
#  (http://www.klayout.de/index.php) version 0.26.1 or later on different Apple
#  ßMac OSX platforms.
#
#  This script must be copied to a "*.macQAT/" directory to run.
#===============================================================================
from __future__ import print_function  # to use print() of Python 3 in Python >= 2.7
import sys
import os
import datetime
from   time import sleep
import six
import platform
import optparse
import subprocess

#-------------------------------------------------------------------------------
## To set global variables including present directory and platform info.
#
#-------------------------------------------------------------------------------
def SetGlobals():
    global ProjectDir         # project directory where "ut_runner" exists
    global RunnerUsage        # True to print the usage of 'ut_runner'
    global Run                # True to run this script
    global ContinueOnError    # True to continue after an error
    global TestsExcluded      # list of tests to exclude
    global Arguments          # other arguments
    global GitSHA1            # Git's short SHA1 value of the HEAD
    global TimeStamp          # time stamp
    global WorkDir            # work directory name
    global LogFile            # log file name
    global Usage              # string on usage
    # auxiliary variables on platform
    global System             # 6-tuple from platform.uname()
    global Node               # - do -
    global Release            # - do -
    global Version            # - do -
    global Machine            # - do -
    global Processor          # - do -
    global Bit                # machine bit-size

    Usage  = "\n"
    Usage += "----------------------------------------------------------------------------------------\n"
    Usage += "<< Usage of 'macQAT.py' >>\n"
    Usage += "       for running 'ut_runner' after building KLayout.\n"
    Usage += "\n"
    Usage += "$ [python] ./macQAT.py \n"
    Usage += "   option & argument      : descriptions                            | default value\n"
    Usage += "   -----------------------------------------------------------------+---------------\n"
    Usage += "   [-u|--usage]           : print usage of 'ut_runner'and exit      | disabled \n"
    Usage += "                                                                    | \n"
    Usage += "   <-r|--run>             : run this script                         | disabled \n"
    Usage += "   [-s|--stop]            : stop on error                           | disabled \n"
    Usage += "   [-x|--exclude <tests>] : exclude test(s) such as 'pymod,pya'     | '' \n"
    Usage += "   [-a|--args <string>]   : arguments other than '-x' and '-c'      | '' \n"
    Usage += "   [-?|--?]               : print this usage and exit               | disabled \n"
    Usage += "--------------------------------------------------------------------+-------------------\n"

    ProjectDir      = os.getcwd()
    RunnerUsage     = False
    Run             = False
    ContinueOnError = True
    TestsExcluded   = list()
    Arguments       = ""
    GitSHA1         = GetGitShortSHA1()
    TimeStamp       = GetTimeStamp()
    WorkDir         = "QATest_%s_%s__%s" % (GitSHA1, TimeStamp, os.path.basename(ProjectDir) )
    LogFile         = WorkDir + ".log"

    (System, Node, Release, Version, Machine, Processor) = platform.uname()

#-------------------------------------------------------------------------------
## Get git's short SHA1 value of the HEAD
#
# @return SHA1 value string
#-------------------------------------------------------------------------------
def GetGitShortSHA1():
    command = "git rev-parse --short HEAD 2>/dev/null"
    sha1val = os.popen( command ).read().strip()
    return sha1val

#-------------------------------------------------------------------------------
## Get the time stamp
#
# @return time stamp string
#-------------------------------------------------------------------------------
def GetTimeStamp():
    ts = datetime.datetime.today()
    return "%04d_%02d%02d_%02d%02d" % (ts.year, ts.month, ts.day, ts.hour, ts.minute)

#-------------------------------------------------------------------------------
## To parse the command line arguments
#
#-------------------------------------------------------------------------------
def ParseCommandLineArguments():
    global Usage
    global RunnerUsage
    global Run
    global ContinueOnError
    global TestsExcluded
    global Arguments

    p = optparse.OptionParser( usage=Usage )
    p.add_option( '-u', '--usage',
                    action='store_true',
                    dest='runner_usage',
                    default=False,
                    help="print usage of 'ut_runner' and exit (false)" )

    p.add_option( '-r', '--run',
                    action='store_true',
                    dest='runme',
                    default=False,
                    help='run this script (false)' )

    p.add_option( '-s', '--stop',
                    action='store_true',
                    dest='stop_on_error',
                    default=False,
                    help='stop on error (false)' )

    p.add_option( '-x', '--exclude',
                    dest='exclude_tests',
                    help="exclude test(s) such as 'pymod,pya' ('')" )

    p.add_option( '-a', '--args',
                    dest='arguments',
                    help="arguments other than '-x' and '-c' ('')" )

    p.add_option( '-?', '--??',
                    action='store_true',
                    dest='checkusage',
                    default=False,
                    help='check usage (false)' )

    p.set_defaults( runner_usage  = False,
                    runme         = False,
                    stop_on_error = False,
                    exclude_tests = "",
                    arguments     = "",
                    checkusage    = False )

    opt, args = p.parse_args()
    if opt.checkusage:
        print(Usage)
        quit()

    RunnerUsage     = opt.runner_usage
    Run             = opt.runme
    ContinueOnError = not opt.stop_on_error
    if not opt.exclude_tests == "":
        TestsExcluded = [ item.strip() for item in opt.exclude_tests.split(',') ]
    else:
        TestsExcluded = []
    Arguments = opt.arguments

#-------------------------------------------------------------------------------
## Hide/Show the private directory
#
#-------------------------------------------------------------------------------
def HidePrivateDir():
    if os.path.isdir( "../private" ):
        os.rename( "../private", "../private.stash"  )

def ShowPrivateDir():
    if os.path.isdir( "../private.stash" ):
        os.rename( "../private.stash", "../private"  )

#-------------------------------------------------------------------------------
## Export environment variables for "ut_runner"
#
#-------------------------------------------------------------------------------
def ExportEnvVariables():
    global ProjectDir
    global WorkDir
    global MyEnviron  # my environment variables; can be independently used later

    # In older versions of subprocess module, 'env=None' argument is not provided
    MyEnviron = os.environ.copy()
    MyEnviron[ 'TESTSRC' ] = ".."
    MyEnviron[ 'TESTTMP' ] = WorkDir
    if System == "Darwin":
        MyEnviron[ 'DYLD_LIBRARY_PATH' ] = "%s:%s/db_plugins" % (ProjectDir, ProjectDir)
        for env in [ 'TESTSRC', 'TESTTMP', 'DYLD_LIBRARY_PATH' ]:
            os.environ[env] = MyEnviron[env]
    else:
        MyEnviron[ 'LD_LIBRARY_PATH' ] = "%s:%s/db_plugins" % (ProjectDir, ProjectDir)
        for env in [ 'TESTSRC', 'TESTTMP', 'LD_LIBRARY_PATH' ]:
            os.environ[env] = MyEnviron[env]

#-------------------------------------------------------------------------------
## Run the tester
#
# @param[in] command  command string to run
# @param[in] logfile  log file name
#-------------------------------------------------------------------------------
def RunTester( command, logfile="" ):
    if six.PY3:
        proc = subprocess.Popen( command.split(),
                                    stdout=subprocess.PIPE,   \
                                    stderr=subprocess.STDOUT, \
                                    universal_newlines=True )
    else:
        proc = subprocess.Popen( command.split(),
                                    stdout=subprocess.PIPE,   \
                                    stderr=subprocess.STDOUT )

    if not logfile == "":
        with proc.stdout, open( logfile, 'w' ) as file:
            for line in proc.stdout:
                sys.stdout.write(line)
                file.write(line)
        proc.wait()
    else:
        with proc.stdout:
            for line in proc.stdout:
                sys.stdout.write(line)
        proc.wait()

#-------------------------------------------------------------------------------
# Main function
#-------------------------------------------------------------------------------
def Main():
    #-------------------------------------------------------
    # [1] Initialize
    #-------------------------------------------------------
    SetGlobals()
    ParseCommandLineArguments()
    ExportEnvVariables()

    #-------------------------------------------------------
    # [2] Print the runner's usage
    #-------------------------------------------------------
    if RunnerUsage:
        command = './ut_runner --help-all'
        RunTester( command )
        quit()

    #-------------------------------------------------------
    # [3] Run the unit tester
    #-------------------------------------------------------
    if not Run:
        print( "! pass <-r|--run> option to run" )
        print(Usage)
        quit()

    command = './ut_runner'
    if ContinueOnError:
        command += " -c"
    for item in TestsExcluded:
        command += ' -x %s' % item
    if not Arguments == "":
        command += " %s" % Arguments

    print( "" )
    print( "### Dumping the log to <%s>" % LogFile )
    print( "------------------------------------------------------------" )
    print( "  Git SHA1   = %s" % GitSHA1 )
    print( "  Time stamp = %s" % TimeStamp )
    print( "------------------------------------------------------------" )
    sleep( 1.0 )
    HidePrivateDir()
    RunTester( command, logfile=LogFile )
    ShowPrivateDir()

#===================================================================================
if __name__ == "__main__":
    Main()

#---------------
# End of file
#---------------
