#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac.py"
#
#  The top Python script for building KLayout (http://www.klayout.de/index.php)
#  version 0.30.2 or later on different Apple Mac OSX platforms.
#===============================================================================
import sys
import os
import re
import codecs
import shutil
import glob
import platform
import optparse
import subprocess
import pprint

#-------------------------------------------------------------------------------
## To import global dictionaries of different modules and utility functions
#-------------------------------------------------------------------------------
mydir = os.path.dirname(os.path.abspath(__file__))
sys.path.append( mydir + "/macbuild" )
from build4mac_env  import *
from build4mac_util import *

#-------------------------------------------------------------------------------
## To generate the OS-wise usage strings and the default module set
#
# @param[in] platform   platform name
#
# @return (usage, moduleset)-tuple
#-------------------------------------------------------------------------------
def GenerateUsage(platform):
    if platform.upper() in [ "TAHOE", "SEQUOIA", "SONOMA", "VENTURA", "MONTEREY" ]: # with Xcode [13.1 .. ]
        myQt56    = "qt5macports"
        myRuby    = "sys"
        myPython  = "sys"
        moduleset = ('Qt5MacPorts', 'Sys', 'Sys')
    else: # too obsolete
        raise Exception( "! Too obsolete platform <%s>" % platform )

    usage  = "\n"
    usage += "-----------------------------------------------------------------------------------------------------------\n"
    usage += "<< Usage of 'build4mac.py' >>\n"
    usage += "       for building KLayout 0.30.2 or later on different Apple macOS platforms.\n"
    usage += "\n"
    usage += "$ [python] ./build4mac.py\n"
    usage += "   option & argument    : descriptions (refer to 'macbuild/build4mac_env.py' for details)  | default value\n"
    usage += "   ----------------------------------------------------------------------------------------+---------------\n"
    usage += "   [-q|--qt <type>]     : case-insensitive type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3',      | %s\n" % myQt56
    usage += "                        :                        'Qt6MacPorts', 'Qt6Brew']                 |\n"
    usage += "                        :   Qt5MacPorts: use Qt5 from MacPorts                             |\n"
    usage += "                        :       Qt5Brew: use Qt5 from Homebrew                             |\n"
    usage += "                        :       Qt5Ana3: use Qt5 from Anaconda3                            |\n"
    usage += "                        :   Qt6MacPorts: use Qt6 from MacPorts (*)                         |\n"
    usage += "                        :       Qt6Brew: use Qt6 from Homebrew (*)                         |\n"
    usage += "                        :                        (*) migration to Qt6 is ongoing           |\n"
    usage += "   [-r|--ruby <type>]   : case-insensitive type=['nil', 'Sys', 'MP33', 'HB34', 'Ana3']     | %s\n" % myRuby
    usage += "                        :    nil: don't bind Ruby                                          |\n"
    usage += "                        :    Sys: use [Sequoia|Sonoma|Ventura|Monterey]-bundled Ruby 2.6   |\n"
    usage += "                        :   MP33: use Ruby 3.3 from MacPorts                               |\n"
    usage += "                        :   HB34: use Ruby 3.4 from Homebrew                               |\n"
    usage += "                        :   Ana3: use Ruby 3.2 from Anaconda3                              |\n"
    usage += "   [-p|--python <type>] : case-insensitive type=['nil', 'Sys', 'MP312', 'HB312', 'Ana3',   | %s\n" % myPython
    usage += "                        :                        'MP311', 'HB311', 'HBAuto']               |\n"
    usage += "                        :    nil: don't bind Python                                        |\n"
    usage += "                        :    Sys: use [Sequoia|Sonoma|Ventura|Monterey]-bundled Python 3.9 |\n"
    usage += "                        :  MP312: use Python 3.12 from MacPorts                            |\n"
    usage += "                        :  HB312: use Python 3.12 from Homebrew                            |\n"
    usage += "                        :   Ana3: use Python 3.12 from Anaconda3                           |\n"
    usage += "                        :  MP311: use Python 3.11 from MacPorts                            |\n"
    usage += "                        :  HB311: use Python 3.11 from Homebrew (+)                        |\n"
    usage += "                        :               (+) required to provide the legacy pip in HW-*.dmg |\n"
    usage += "                        : HBAuto: use the latest Python 3.x auto-detected from Homebrew    |\n"
    usage += "   [-P|--buildPymod]    : build and deploy Pymod (*.whl) for LW-*.dmg                      | disabled\n"
    usage += "   [-n|--noqtbinding]   : don't create Qt bindings for ruby scripts                        | disabled\n"
    usage += "   [-u|--noqtuitools]   : don't include uitools in Qt binding                              | disabled\n"
    usage += "   [-g|--nolibgit2]     : don't include libgit2 for Git package support                    | disabled\n"
    usage += "   [-m|--make <option>] : option passed to 'make'                                          | '--jobs=4'\n"
    usage += "   [-d|--debug]         : enable debug mode build; AddressSanitizer (ASAN) is linked       | disabled\n"
    usage += "   [-c|--checkcom]      : check command-line and exit without building                     | disabled\n"
    usage += "   [-y|--deploy]        : deploy executables and dylibs, including Qt's Frameworks         | disabled\n"
    usage += "   [-Y|--DEPLOY]        : deploy executables and dylibs for those who built KLayout        | disabled\n"
    usage += "                        : from the source code and use the tools in the same machine       |\n"
    usage += "                        : ! After confirmation of the successful build of 'klayout.app',   |\n"
    usage += "                        :   rerun this script with BOTH:                                   |\n"
    usage += "                        :     1) the same options used for building AND                    |\n"
    usage += "                        :     2) <-y|--deploy> OR <-Y|--DEPLOY>                            |\n"
    usage += "                        :   optionally with [-v|--verbose <0-3>]                           |\n"
    usage += "   [-v|--verbose <0-3>] : verbose level of `macdeployqt' (effective with -y only)          | 1\n"
    usage += "                        : 0 = no output, 1 = error/warning (default),                      |\n"
    usage += "                        : 2 = normal,    3 = debug                                         |\n"
    usage += "   [-?|--?]             : print this usage and exit; in zsh, quote like '-?' or '--?'      | disabled\n"
    usage += "-------------------------------------------------------------------------------------------+---------------\n"
    return (usage, moduleset)

#-------------------------------------------------------------------------------
## To get the default configurations
#
# @return a dictionary containing the default configurations for the macOS build
#-------------------------------------------------------------------------------
def Get_Default_Config():
    ProjectDir = os.getcwd()
    BuildBash  = "./build.sh"
    (System, Node, Release, MacVersion, Machine, Processor) = platform.uname()

    if not System == "Darwin":
        print("")
        print( "!!! Sorry. Your system <%s> looks like non-Mac" % System, file=sys.stderr )
        sys.exit(1)

    release = int( Release.split(".")[0] ) # take the first of ['21', '0', '0']
    #----------------------------------------------------------------------------
    # Dropped [ElCapitan - BigSur] (2023-10-24).
    # See 415b5aa2efca04928f1148a69e77efd5d76f8c1d for the previous states.
    #----------------------------------------------------------------------------
    if   release == 25:
        Platform = "Tahoe"
    elif release == 24:
        Platform = "Sequoia"
    elif release == 23:
        Platform = "Sonoma"
    elif release == 22:
        Platform = "Ventura"
    elif release == 21:
        Platform = "Monterey"
    else:
        Platform = ""
        print("")
        print( "!!! Sorry. Unsupported major OS release <%d>" % release, file=sys.stderr )
        print( GenerateUsage("")[0] )
        sys.exit(1)

    if not Machine == "x86_64":
		# with an Apple Silicon Chip?
        if Machine == "arm64" and Platform in ["Tahoe", "Sequoia", "Sonoma", "Ventura", "Monterey"]:
            print("")
            print( "### Your Mac equips an Apple Silicon Chip ###" )
            print( "    Setting QMAKE_APPLE_DEVICE_ARCHS=arm64\n")
            os.environ['QMAKE_APPLE_DEVICE_ARCHS'] = 'arm64'
        else:
            print("")
            print( "!!! Sorry. Only x86_64/arm64 architecture machine is supported but found <%s>" % Machine, file=sys.stderr )
            print( GenerateUsage("")[0] )
            sys.exit(1)

    # Set the OS-wise usage and module set
    Usage, ModuleSet = GenerateUsage(Platform)

    # developer's debug level list for this tool
    ToolDebug = list()

    # Set the default modules
    if   Platform == "Tahoe":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "Sys"
        ModulePython = "Sys"
    elif Platform == "Sequoia":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "Sys"
        ModulePython = "Sys"
    elif Platform == "Sonoma":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "Sys"
        ModulePython = "Sys"
    elif Platform == "Ventura":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "Sys"
        ModulePython = "Sys"
    elif Platform == "Monterey":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "Sys"
        ModulePython = "Sys"
    else:
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "nil"
        ModulePython = "nil"

    BuildPymodWhl = False
    NonOSStdLang  = False
    NoQtBindings  = False
    NoQtUiTools   = False
    NoLibGit2     = False
    MakeOptions   = "--jobs=4"
    DebugMode     = False
    CheckComOnly  = False
    DeploymentF   = False
    DeploymentP   = False
    PackagePrefix = ""
    DeployVerbose = 1
    Version       = GetKLayoutVersionFrom( "./version.sh" )
    OSPython3FW   = None  # system Python3 frameworks in [ None, MontereyPy3FW, VenturaPy3FW, SonomaPy3FW, SequoiaPy3FW, TahoePy3FW ]
    EmbedQt       = False
    EmbedPython3  = False

    config = dict()
    config['ProjectDir']    = ProjectDir        # project directory where "build.sh" exists
    config['Usage']         = Usage             # string on usage
    config['BuildBash']     = BuildBash         # the main build Bash script
    config['Platform']      = Platform          # platform
    config['ModuleQt']      = ModuleQt          # Qt module to be used
    config['ModuleRuby']    = ModuleRuby        # Ruby module to be used
    config['ModulePython']  = ModulePython      # Python module to be used
    config['BuildPymodWhl'] = BuildPymodWhl     # True to build and deploy "Pymod (*.whl)"
    config['NonOSStdLang']  = NonOSStdLang      # True if non-OS-standard language is chosen
    config['NoQtBindings']  = NoQtBindings      # True if not creating Qt bindings for Ruby scripts
    config['NoQtUiTools']   = NoQtUiTools       # True if not to include QtUiTools in Qt binding
    config['NoLibGit2']     = NoLibGit2         # True if not to include libgit2 for Git package support
    config['MakeOptions']   = MakeOptions       # options passed to `make`
    config['DebugMode']     = DebugMode         # True if debug mode build
    config['CheckComOnly']  = CheckComOnly      # True if only for checking the command line parameters to "build.sh"
    config['DeploymentF']   = DeploymentF       # True if fully (including Qt's Frameworks) deploy the binaries for bundles
    config['DeploymentP']   = DeploymentP       # True if partially deploy the binaries excluding Qt's Frameworks
    config['PackagePrefix'] = PackagePrefix     # the package prefix: 'ST-', 'LW-', 'HW-', or 'EX-'
    config['DeployVerbose'] = DeployVerbose     # -verbose=<0-3> level passed to 'macdeployqt' tool
    config['Version']       = Version           # KLayout's version
    config['ModuleSet']     = ModuleSet         # (Qt, Ruby, Python)-tuple
    config['ToolDebug']     = ToolDebug         # debug level list for this tool
    config['OSPython3FW']   = OSPython3FW       # system Python3 frameworks in [ None, MontereyPy3FW, VenturaPy3FW, SonomaPy3FW, SequoiaPy3FW, TahoePy3FW ]
    config['EmbedQt']       = EmbedQt           # True if Qt is embedded
    config['EmbedPython3']  = EmbedPython3      # True if Python3 is embedded
    # auxiliary variables on platform
    config['System']        = System            # 6-tuple from platform.uname()
    config['Node']          = Node              # - do -
    config['Release']       = Release           # - do -
    config['MacVersion']    = MacVersion        # - do -
    config['Machine']       = Machine           # - do -
    config['Processor']     = Processor         # - do -
    return config

#------------------------------------------------------------------------------
## To parse the command line parameters
#
# @param[in] config     dictionary containing the default configuration
#
# @return the configuration dictionary updated with the CLI parameters
#------------------------------------------------------------------------------
def Parse_CLI_Args(config):
    #-----------------------------------------------------
    # [1] Retrieve the configuration
    #-----------------------------------------------------
    Usage         = config['Usage']
    Platform      = config['Platform']
    Release       = config['Release']
    Machine       = config['Machine']
    ModuleQt      = config['ModuleQt']
    ModuleRuby    = config['ModuleRuby']
    ModulePython  = config['ModulePython']
    BuildPymodWhl = config['BuildPymodWhl']
    NonOSStdLang  = config['NonOSStdLang']
    NoQtBindings  = config['NoQtBindings']
    NoQtUiTools   = config['NoQtUiTools']
    NoLibGit2     = config['NoLibGit2']
    MakeOptions   = config['MakeOptions']
    DebugMode     = config['DebugMode']
    CheckComOnly  = config['CheckComOnly']
    DeploymentF   = config['DeploymentF']
    DeploymentP   = config['DeploymentP']
    PackagePrefix = config['PackagePrefix']
    DeployVerbose = config['DeployVerbose']
    ModuleSet     = config['ModuleSet']
    ToolDebug     = config['ToolDebug']
    OSPython3FW   = config['OSPython3FW']
    EmbedQt       = config['EmbedQt']
    EmbedPython3  = config['EmbedPython3']

    #-----------------------------------------------------
    # [2] Parse the CLI arguments
    #-----------------------------------------------------
    p = optparse.OptionParser(usage=Usage)
    p.add_option( '-q', '--qt',
                    dest='type_qt',
                    help="Qt type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3', 'Qt6MacPorts', 'Qt6Brew']" )

    p.add_option( '-r', '--ruby',
                    dest='type_ruby',
                    help="Ruby type=['nil', 'Sys', 'MP33', 'HB34', 'Ana3']" )

    p.add_option( '-p', '--python',
                    dest='type_python',
                    help="Python type=['nil', 'Sys', 'MP312', 'HB312', 'Ana3', 'MP311', 'HB311', 'HBAuto']" )

    p.add_option( '-P', '--buildPymod',
                    action='store_true',
                    dest='build_pymod_whl',
                    default=False,
                    help="build and deploy Pymod (*.whl) (disabled)" )

    p.add_option( '-n', '--noqtbinding',
                    action='store_true',
                    dest='no_qt_binding',
                    default=False,
                    help="do not create Qt bindings for Ruby scripts" )

    p.add_option( '-u', '--noqtuitools',
                    action='store_true',
                    dest='no_qt_uitools',
                    default=False,
                    help="don't include uitools in Qt binding" )

    p.add_option( '-g', '--nolibgit2',
                    action='store_true',
                    dest='no_libgit2',
                    default=False,
                    help="don't include libgit2 for Git package support" )

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

    p.add_option( '-y', '--deploy',
                    action='store_true',
                    dest='deploy_full',
                    default=False,
                    help="fully deploy built binaries" )

    p.add_option( '-Y', '--DEPLOY',
                    action='store_true',
                    dest='deploy_partial',
                    default=False,
                    help="deploy built binaries when non-OS-standard script language is chosen" )

    p.add_option( '-v', '--verbose',
                    dest='deploy_verbose',
                    help="verbose level of `macdeployqt` tool" )

    p.add_option( '-t', '--tooldebug',
                    action='append',
                    dest='tool_debug',
                    help="developer's debug level list for this tool" ) # not shown in the usage

    p.add_option( '-?', '--??',
                    action='store_true',
                    dest='checkusage',
                    default=False,
                    help='check usage' )

    if Platform.upper() in [ "TAHOE", "SEQUOIA", "SONOMA", "VENTURA", "MONTEREY" ]: # with Xcode [13.1 .. ]
        p.set_defaults( type_qt         = "qt5macports",
                        type_ruby       = "sys",
                        type_python     = "sys",
                        build_pymod_whl = False,
                        no_qt_binding   = False,
                        no_qt_uitools   = False,
                        no_libgit2      = False,
                        make_option     = "--jobs=4",
                        debug_build     = False,
                        check_command   = False,
                        deploy_full     = False,
                        deploy_partial  = False,
                        deploy_verbose  = "1",
                        tool_debug      = [],
                        checkusage      = False )
    else:
        raise Exception( "! Too obsolete platform <%s>" % Platform )

    opt, args = p.parse_args()
    if (opt.checkusage):
        print(Usage)
        sys.exit(0)

    # (A) Determine the Qt type
    candidates                = dict()
    candidates['QT5MACPORTS'] = 'Qt5MacPorts'
    candidates['QT5BREW']     = 'Qt5Brew'
    candidates['QT5ANA3']     = 'Qt5Ana3'
    candidates['QT6MACPORTS'] = 'Qt6MacPorts'
    candidates['QT6BREW']     = 'Qt6Brew'
    try:
        ModuleQt = candidates[ opt.type_qt.upper() ]
    except KeyError:
        ModuleQt = ''
        pass
    if ModuleQt == '':
        print("")
        print( "!!! Unknown Qt type <%s>. Case-insensitive candidates: %s" % \
                        (opt.type_qt, list(candidates.keys())), file=sys.stderr )
        print(Usage)
        sys.exit(1)
    elif ModuleQt == "Qt5MacPorts":
        choiceQt56 = 'qt5MP'
    elif ModuleQt == "Qt5Brew":
        choiceQt56 = 'qt5Brew'
    elif ModuleQt == "Qt5Ana3":
        choiceQt56 = 'qt5Ana3'
    elif ModuleQt == "Qt6MacPorts":
        choiceQt56 = 'qt6MP'
    elif ModuleQt == "Qt6Brew":
        choiceQt56 = 'qt6Brew'

    # Check if non-OS-standard (-bundled) script languages (Ruby and Python) are used
    NonOSStdLang = False

    # (B) Determine the Ruby type
    candidates         = dict()
    candidates['NIL']  = 'nil'
    candidates['SYS']  = 'Sys'
    candidates['MP33'] = 'MP33'
    candidates['HB34'] = 'HB34'
    candidates['ANA3'] = 'Ana3'
    try:
        choiceRuby = candidates[ opt.type_ruby.upper() ]
    except KeyError:
        ModuleRuby = ''
        pass
    else:
        ModuleRuby = ''
        if choiceRuby == "nil":
            ModuleRuby = 'nil'
        elif choiceRuby == "Sys":
            if Platform == "Tahoe":
                ModuleRuby = 'RubyTahoe'
            elif Platform == "Sequoia":
                ModuleRuby = 'RubySequoia'
            elif Platform == "Sonoma":
                ModuleRuby = 'RubySonoma'
            elif Platform == "Ventura":
                ModuleRuby = 'RubyVentura'
            elif Platform == "Monterey":
                ModuleRuby = 'RubyMonterey'
        elif choiceRuby == "MP33":
            ModuleRuby   = 'Ruby33MacPorts'
            NonOSStdLang = True
        elif choiceRuby == "HB34":
            ModuleRuby   = 'Ruby34Brew'
            NonOSStdLang = True
        elif choiceRuby == "Ana3":
            ModuleRuby   = 'RubyAnaconda3'
            NonOSStdLang = True
    if ModuleRuby == '':
        print("")
        print( "!!! Unknown Ruby type <%s>. Case-insensitive candidates: %s" % \
                        (opt.type_ruby, list(candidates.keys())), file=sys.stderr )
        print(Usage)
        sys.exit(1)

    # (C) Determine the Python type
    candidates           = dict()
    candidates['NIL']    = 'nil'
    candidates['SYS']    = 'Sys'
    candidates['MP312']  = 'MP312'
    candidates['HB312']  = 'HB312'
    candidates['ANA3']   = 'Ana3'
    candidates['MP311']  = 'MP311'
    candidates['HB311']  = 'HB311'
    candidates['HBAUTO'] = 'HBAuto'
    try:
        choicePython = candidates[ opt.type_python.upper() ]
    except KeyError:
        ModulePython = ''
        pass
    else:
        ModulePython = ''
        if choicePython ==  "nil":
            ModulePython = 'nil'
            OSPython3FW  = None
        elif choicePython == "Sys":
            if Platform == "Tahoe":
                ModulePython = 'PythonTahoe'
                OSPython3FW  = TahoePy3FW
            elif Platform == "Sequoia":
                ModulePython = 'PythonSequoia'
                OSPython3FW  = SequoiaPy3FW
            elif Platform == "Sonoma":
                ModulePython = 'PythonSonoma'
                OSPython3FW  = SonomaPy3FW
            elif Platform == "Ventura":
                ModulePython = 'PythonVentura'
                OSPython3FW  = VenturaPy3FW
            elif Platform == "Monterey":
                ModulePython = 'PythonMonterey'
                OSPython3FW  = MontereyPy3FW
        elif choicePython == "MP312":
            ModulePython = 'Python312MacPorts'
            OSPython3FW  = None
            NonOSStdLang = True
        elif choicePython == "HB312":
            ModulePython = 'Python312Brew'
            OSPython3FW  = None
            NonOSStdLang = True
        elif choicePython == "Ana3":
            ModulePython = 'PythonAnaconda3'
            OSPython3FW  = None
            NonOSStdLang = True
        elif choicePython == "HB311":
            ModulePython = 'Python311Brew'
            OSPython3FW  = None
            NonOSStdLang = True
        elif choicePython == "HBAuto":
            ModulePython = 'PythonAutoBrew'
            OSPython3FW  = None
            NonOSStdLang = True
    if ModulePython == '':
        print("")
        print( "!!! Unknown Python type <%s>. Case-insensitive candidates: %s" % \
                        (opt.type_python, list(candidates.keys())), file=sys.stderr )
        print(Usage)
        sys.exit(1)

    # (D) Set of modules chosen
    ModuleSet = ( choiceQt56, choiceRuby, choicePython )

    # (E) Set other parameters
    BuildPymodWhl = opt.build_pymod_whl
    NoQtBindings  = opt.no_qt_binding
    NoQtUiTools   = opt.no_qt_uitools
    NoLibGit2     = opt.no_libgit2
    MakeOptions   = opt.make_option
    DebugMode     = opt.debug_build
    CheckComOnly  = opt.check_command
    DeploymentF   = opt.deploy_full
    DeploymentP   = opt.deploy_partial
    ToolDebug     = sorted( set([ int(val) for val in opt.tool_debug ]) )

    if DeploymentF and DeploymentP:
        print("")
        print( "!!! Choose either [-y|--deploy] or [-Y|--DEPLOY]", file=sys.stderr )
        print(Usage)
        sys.exit(1)

    DeployVerbose = int(opt.deploy_verbose)
    if not DeployVerbose in [0, 1, 2, 3]:
        print("")
        print( "!!! Unsupported verbose level passed to `macdeployqt` tool", file=sys.stderr )
        print(Usage)
        sys.exit(1)

    # (F) Build mode
    if not DeploymentF and not DeploymentP:
        target  = "%s %s %s" % (Platform, Release, Machine)
        modules = "Qt=%s, Ruby=%s, Python=%s" % (ModuleQt, ModuleRuby, ModulePython)
        if BuildPymodWhl:
            pymodWhlbuild = "enabled"
        else:
            pymodWhlbuild = "disabled"
        message = "### You are going to build KLayout\n    for  <%s>\n    with <%s>\n    with Pymod <%s>...\n"
        print("")
        print( message % (target, modules, pymodWhlbuild) )
    # (G) Deploy mode
    else:
        EmbedQt      = False
        EmbedPython3 = False
        okHWdmg      = True
        message      = "### You are going to make "
        if DeploymentP:
            PackagePrefix = "LW-"
            if not BuildPymodWhl:
                message += "a lightweight (LW-) package excluding Qt[5|6], Ruby, and Python..."
            else:
                message += "a lightweight (LW-) package with Pymod (*.whl) excluding Qt[5|6], Ruby, and Python..."
        elif DeploymentF:
            if  ModuleQt == "Qt5Ana3":
                EmbedQt  = False
                message += "Qt5 from Anaconda3 embedded, which is not allowed!"
                okHWdmg  = False
            elif (ModuleRuby in RubySys) and (ModulePython in PythonSys):
                PackagePrefix = "ST-"
                EmbedQt       = True
                message      += "a standard (ST-) package including Qt[5|6] and using OS-bundled Ruby and Python..."
            elif ModulePython in ['Python311Brew', 'PythonAutoBrew']:
                PackagePrefix = "HW-"
                EmbedQt       = True
                EmbedPython3  = True
                message      += "a heavyweight (HW-) package including Qt[5|6] and Python3.[11] from Homebrew..."
                okHWdmg = (ModulePython == 'Python311Brew') or \
                          (ModulePython == 'PythonAutoBrew' and HBPythonAutoVersion in ["3.11"] )
            else:
                PackagePrefix = "EX-"
                message      += "a package with exceptional (EX-) combinations of different modules..."
        print( "" )
        print( message )
        print( "" )
        if not okHWdmg:
            print( "!!! HW-dmg package assumes the two conditions:" )
            print( "      (1) either Qt5 or Qt6 from MacPorts or Homebrew (Anaconda3 is not a candidate)" )
            print( "      (2) python@3.11 from Homebrew" )
            sys.exit(1)
        if CheckComOnly:
            sys.exit(0)

    #-----------------------------------------------------
    # [3] Update the configuration to return
    #-----------------------------------------------------
    config['Usage']         = Usage
    config['Platform']      = Platform
    config['ModuleQt']      = ModuleQt
    config['ModuleRuby']    = ModuleRuby
    config['ModulePython']  = ModulePython
    config['BuildPymodWhl'] = BuildPymodWhl
    config['NonOSStdLang']  = NonOSStdLang
    config['NoQtBindings']  = NoQtBindings
    config['NoQtUiTools']   = NoQtUiTools
    config['NoLibGit2']     = NoLibGit2
    config['MakeOptions']   = MakeOptions
    config['DebugMode']     = DebugMode
    config['CheckComOnly']  = CheckComOnly
    config['DeploymentF']   = DeploymentF
    config['DeploymentP']   = DeploymentP
    config['PackagePrefix'] = PackagePrefix
    config['DeployVerbose'] = DeployVerbose
    config['ModuleSet']     = ModuleSet
    config['ToolDebug']     = ToolDebug
    config['OSPython3FW']   = OSPython3FW
    config['EmbedQt']       = EmbedQt
    config['EmbedPython3']  = EmbedPython3

    if CheckComOnly:
        pp = pprint.PrettyPrinter( indent=4, width=140 )
        parameters = Get_Build_Parameters(config)
        Build_pymod_wheel(parameters)
        pp.pprint(parameters)
        sys.exit(0)
    else:
        return config

#------------------------------------------------------------------------------
## To run the main Bash script "build.sh" with appropriate options
#
# @param[in] config     dictionary containing the build configuration

# @return a dictionary containing the build parameters
#------------------------------------------------------------------------------
def Get_Build_Parameters(config):
    #-----------------------------------------------------
    # [1] Retrieve the configuration
    #-----------------------------------------------------
    ProjectDir    = config['ProjectDir']
    Platform      = config['Platform']
    BuildBash     = config['BuildBash']
    ModuleQt      = config['ModuleQt']
    ModuleRuby    = config['ModuleRuby']
    ModulePython  = config['ModulePython']
    BuildPymodWhl = config['BuildPymodWhl']
    ModuleSet     = config['ModuleSet']
    NoQtBindings  = config['NoQtBindings']
    NoQtUiTools   = config['NoQtUiTools']
    NoLibGit2     = config['NoLibGit2']
    MakeOptions   = config['MakeOptions']
    DebugMode     = config['DebugMode']
    CheckComOnly  = config['CheckComOnly']
    DeploymentF   = config['DeploymentF']
    DeploymentP   = config['DeploymentP']
    PackagePrefix = config['PackagePrefix']
    OSPython3FW   = config['OSPython3FW']

    #-----------------------------------------------------
    # [2] Set parameters passed to the main Bash script
    #-----------------------------------------------------
    parameters = dict()
    parameters['build_cmd']      = BuildBash
    parameters['check_cmd_only'] = CheckComOnly

    # (A) debug or release
    parameters['debug_mode'] = DebugMode  # True if debug, False if release
    if parameters["debug_mode"]:
        mode = "debug"
    else:
        mode = "release"

    # (B) Modules
    (qt, ruby, python) = ModuleSet  # ( 'qt6Brew', 'Sys', 'Sys' )
    ruby_python = "R%sP%s" % ( ruby.lower(), python.lower() )

    # (C) Target directories and files
    MacPkgDir             = "%s%s.pkg.macos-%s-%s-%s"     % (PackagePrefix, qt, Platform, mode, ruby_python)
    MacBinDir             = "%s.bin.macos-%s-%s-%s"       % (               qt, Platform, mode, ruby_python)
    MacBuildDir           = "%s.build.macos-%s-%s-%s"     % (               qt, Platform, mode, ruby_python)
    MacBuildLog           = "%s.build.macos-%s-%s-%s.log" % (               qt, Platform, mode, ruby_python)
    MacBuildDirQAT        = MacBuildDir + ".macQAT"
    parameters['bin']     = MacBinDir
    parameters['build']   = MacBuildDir
    parameters['logfile'] = MacBuildLog

    # (D) about Qt[5|6]
    parameters['qmake']       = Qt56Dictionary[ModuleQt]['qmake']
    parameters['deploy_tool'] = Qt56Dictionary[ModuleQt]['deploy']
    parameters['qt_lib_root'] = Qt56Dictionary[ModuleQt]['libdir']

    # (E) rpath
    if OSPython3FW in [ MontereyPy3FW, VenturaPy3FW, SonomaPy3FW, SequoiaPy3FW ]:
        parameters['rpath'] = OSPython3FW
    else:
        parameters['rpath'] = "@executable_path/../Frameworks"

    # (F) want Qt bindings with Ruby scripts?
    parameters['no_qt_bindings'] = NoQtBindings

    # (G) want QtUiTools?
    parameters['no_qt_uitools'] = NoQtUiTools

    # (H) options to `make` tool
    if not MakeOptions == "":
        parameters['make_options'] = MakeOptions
        try:
            jobopt, number = MakeOptions.split('=')  # like '--jobs=4' ?
            pnum = int(number)
        except Exception:
            parameters['num_parallel'] = 4  # default
        else:
            parameters['num_parallel'] = pnum

    # (I) about Ruby
    if ModuleRuby != "nil":
        parameters['ruby']  = RubyDictionary[ModuleRuby]['exe']
        parameters['rbinc'] = RubyDictionary[ModuleRuby]['inc']
        parameters['rblib'] = RubyDictionary[ModuleRuby]['lib']
        if 'inc2' in RubyDictionary[ModuleRuby]:
            parameters['rbinc2'] = RubyDictionary[ModuleRuby]['inc2']

    # (J) about Python
    if ModulePython != "nil":
        parameters['python'] = PythonDictionary[ModulePython]['exe']
        parameters['pyinc']  = PythonDictionary[ModulePython]['inc']
        parameters['pylib']  = PythonDictionary[ModulePython]['lib']

    config['MacPkgDir']      = MacPkgDir        # relative path to package directory
    config['MacBinDir']      = MacBinDir        # relative path to binary directory
    config['MacBuildDir']    = MacBuildDir      # relative path to build directory
    config['MacBuildDirQAT'] = MacBuildDirQAT   # relative path to build directory for QATest
    config['MacBuildLog']    = MacBuildLog      # relative path to build log file

    # (K) Extra parameters needed for deployment
    parameters['project_dir'] = ProjectDir

    # (L) Extra parameters needed for <pymod>
    #     <pymod> will be built if:
    #       BuildPymodWhl = True
    #       Platform      = [ 'Sequoia', 'Sonoma', 'Ventura', 'Monterey']
    #       ModuleRuby    = [ 'Ruby33MacPorts', 'Ruby34Brew', 'RubyAnaconda3' ]
    #       ModulePython  = [ 'Python312MacPorts', 'Python311MacPorts',
    #                         'Python311Brew',
    #                         'PythonAnaconda3' ]
    parameters['BuildPymodWhl'] = BuildPymodWhl
    parameters['Platform']      = Platform
    parameters['ModuleRuby']    = ModuleRuby
    parameters['ModulePython']  = ModulePython

    PymodDistDir = dict()
    if Platform in [ 'Tahoe', 'Sequoia', 'Sonoma', 'Ventura', 'Monterey' ]:
        if ModuleRuby in [ 'Ruby33MacPorts', 'Ruby34Brew', 'RubyAnaconda3' ]:
            if ModulePython in [ 'Python312MacPorts', 'Python311MacPorts' ]:
                PymodDistDir[ModulePython] = 'dist-MP3-%s' % ModuleQt
            elif ModulePython in [ 'Python311Brew' ]:
                PymodDistDir[ModulePython] = 'dist-HB3-%s' % ModuleQt
            elif ModulePython in [ 'PythonAnaconda3' ]:
                PymodDistDir[ModulePython] = 'dist-ana3-%s' % ModuleQt
    parameters['pymod_dist'] = PymodDistDir
    return parameters

#------------------------------------------------------------------------------
## To run the "setup.py" script with appropriate options for building
#  the klayout Python Module "pymod (*.whl)".
#
# @param[in] parameters     dictionary containing the build parameters
#
# @return 0 on success; non-zero (1), otherwise
#------------------------------------------------------------------------------
def Build_pymod_wheel(parameters):
    #-----------------------------------------------------------------------------------------------------------
    # [1] <pymod> will be built if:
    #       BuildPymodWhl = True
    #       Platform      = [ 'Sequoia', 'Sonoma', 'Ventura', 'Monterey']
    #       ModuleRuby    = [ 'Ruby33MacPorts', 'Ruby34Brew', 'RubyAnaconda3' ]
    #       ModulePython  = [ 'Python312MacPorts', 'Python311MacPorts',
    #                         'Python311Brew',
    #                         'PythonAnaconda3' ]
    #-----------------------------------------------------------------------------------------------------------
    BuildPymodWhl = parameters['BuildPymodWhl']
    Platform      = parameters['Platform']
    ModuleRuby    = parameters['ModuleRuby']
    ModulePython  = parameters['ModulePython']
    if not BuildPymodWhl:
        return 0
    if not Platform in [ 'Tahoe', 'Sequoia', 'Sonoma', 'Ventura', 'Monterey' ]:
        return 0
    elif not ModuleRuby in [ 'Ruby33MacPorts', 'Ruby34Brew', 'RubyAnaconda3' ]:
        return 0
    elif not ModulePython in [ 'Python312MacPorts', 'Python311MacPorts', \
                               'Python311Brew', \
                               'PythonAnaconda3' ]:
        return 0

    #--------------------------------------------------------------------
    # [2] Get the new directory names (dictionary) for "dist" and
    #     set the CPATH environment variable for including <png.h>
    #     required to build the pymod of 0.28 or later
    #--------------------------------------------------------------------
    PymodDistDir = parameters['pymod_dist']
    # Using MacPorts
    if PymodDistDir[ModulePython].find('dist-MP3') >= 0:
        addBinPath = "/opt/local/bin"
        addIncPath = "/opt/local/include"
        addLibPath = "/opt/local/lib"
        whlTarget  = "MP3"
    # Using Homebrew
    elif PymodDistDir[ModulePython].find('dist-HB3') >= 0:
        addBinPath = "%s/bin"     % DefaultHomebrewRoot  # defined in "build4mac_env.py"
        addIncPath = "%s/include" % DefaultHomebrewRoot  # -- ditto --
        addLibPath = "%s/lib"     % DefaultHomebrewRoot  # -- ditto --
        whlTarget  = "HB3"
    # Using Anaconda3
    elif  PymodDistDir[ModulePython].find('dist-ana3') >= 0:
        addBinPath = "/Applications/anaconda3/bin"
        addIncPath = "/Applications/anaconda3/include"
        addLibPath = "/Applications/anaconda3/lib"
        whlTarget  = "ana3"
    else:
        addBinPath = ""
        addIncPath = ""
        addLibPath = ""
        whlTarget  = ""

    if not addBinPath == "":
        try:
            bpath = os.environ['PATH']
        except KeyError:
            os.environ['PATH'] = addBinPath
        else:
            os.environ['PATH'] = "%s:%s" % (addBinPath, bpath)

    if not addIncPath == "":
        try:
            cpath = os.environ['CPATH']
        except KeyError:
            os.environ['CPATH'] = addIncPath
        else:
            os.environ['CPATH'] = "%s:%s" % (addIncPath, cpath)

    if not addLibPath == "":
        try:
            ldpath = os.environ['LDFLAGS']
        except KeyError:
            os.environ['LDFLAGS'] = '-L%s -headerpad_max_install_names' % addLibPath
        else:
            os.environ['LDFLAGS'] = '-L%s %s -headerpad_max_install_names' % (addLibPath, ldpath)

    #--------------------------------------------------------------------
    # [3] Set different command line parameters for building <pymod>
    #--------------------------------------------------------------------
    cmd1_args = "   -m setup  build \\\n"
    cmd2_args = "   -m setup  bdist_wheel \\\n"
    deloc_cmd = "   delocate-wheel --ignore-missing-dependencies"
    cmd3_args = "   <wheel file> \\\n"
    cmd4_args = "   -m setup  clean --all \\\n"

    #-----------------------------------------------------------------------------------
    # [4] Make the consolidated command lines
    #     "caffeinate" makes the CPU run at full speed even when the screen is locked.
    #-----------------------------------------------------------------------------------
    command1  = "time"
    command1 += " \\\n   caffeinate -i %s \\\n" % parameters['python']
    command1 += cmd1_args
    command1 += "   2>&1 | tee -a %s; \\\n" % parameters['logfile']
    command1 += "   test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

    command2  = "time"
    command2 += " \\\n   caffeinate -i %s \\\n" % parameters['python']
    command2 += cmd2_args
    command2 += "   2>&1 | tee -a %s; \\\n" % parameters['logfile']
    command2 += "   test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

    command3  = "time"
    command3 += " \\\n   caffeinate -i %s \\\n" % deloc_cmd
    command3 += cmd3_args
    command3 += "   2>&1 | tee -a %s; \\\n" % parameters['logfile']
    command3 += "   test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

    command4  = "time"
    command4 += " \\\n   caffeinate -i %s \\\n" % parameters['python']
    command4 += cmd4_args
    command4 += "   2>&1 | tee -a %s; \\\n" % parameters['logfile']
    command4 += "   test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

    print( "" )
    print( "### You are going to build <pymod> with the following four stages." )
    print( "<Stage-1>")
    print( "  ", command1 )
    print( "" )

    print( "<Stage-2>")
    print( "  ", command2 )
    print( "" )

    print( "<Stage-3>")
    print( "  ", command3 )
    print( "" )

    print( "<Stage-4>")
    print( "  ", command4 )
    print( "" )

    if parameters['check_cmd_only']:
        return 0

    #-----------------------------------------------------
    # [5-A] Invoke the main Python scripts; takes time:-)
    #-----------------------------------------------------
    myscript = os.path.basename(__file__)
    ret = subprocess.call( command1, shell=True )
    if ret != 0:
        print( "", file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "!!! <%s>: failed to build <pymod>" % myscript, file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "", file=sys.stderr )
        return 1

    ret = subprocess.call( command2, shell=True )
    if ret != 0:
        print( "", file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "!!! <%s>: failed to build <pymod-wheel>" % myscript, file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "", file=sys.stderr )
        return 1

    #---------------------------------------------------------------------------------------------------------
    # [5-B] Copy and relink library dependencies for wheel.
    #       In this step, the "delocate-wheel" command using the desired Python must be found in the PATH.
    #       Refer to: https://github.com/Kazzz-S/klayout/issues/49#issuecomment-1432154118
    #                 https://pypi.org/project/delocate/
    #---------------------------------------------------------------------------------------------------------
    cmd3_args = glob.glob( "dist/*.whl" )  # like ['dist/klayout-0.30.2-cp312-cp312-macosx_10_15_x86_64.whl']
    if len(cmd3_args) == 1:
        command3  = "time"
        command3 += " \\\n   caffeinate -i %s \\\n" % deloc_cmd
        command3 += "  %s \\\n" % cmd3_args[0]
        command3 += "   2>&1 | tee -a %s; \\\n" % parameters['logfile']
        command3 += "   test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0
        ret = subprocess.call( command3, shell=True )
    else:
        ret = 1
    if ret != 0:
        print( "", file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "!!! <%s>: failed to <delocate-wheel>" % myscript, file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "", file=sys.stderr )
        return 1

    ret = subprocess.call( command4, shell=True )
    if ret != 0:
        print( "", file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "!!! <%s>: failed to clean <pymod>" % myscript, file=sys.stderr )
        print( "-------------------------------------------------------------", file=sys.stderr )
        print( "", file=sys.stderr )
        return 1

    #------------------------------------------------------------------------
    # [5-C] Forcibly change the wheel file name for anaconda3
    #       Ref. https://github.com/Kazzz-S/klayout/issues/53
    #         original: klayout-0.30.2-cp312-cp312-macosx_10_15_x86_64.whl
    #               |
    #               V
    #              new: klayout-0.30.2-cp312-cp312-macosx_10_9_x86_64.whl
    #------------------------------------------------------------------------
    if whlTarget == "ana3":
        wheels = glob.glob( "dist/*.whl" )  # like ['dist/klayout-0.30.2-cp312-cp312-macosx_10_15_x86_64.whl']
        if not len(wheels) == 1:
            print( "", file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to <find wheel for anaconda3>" % myscript, file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            return 1
        else:
            pass

        original = wheels[0]
        #                0             1     2     3      4     5         6           *7            8         9
        patwhl = r"(^dist/klayout-)([0-9.]+)(-)(cp[0-9]+)(-)(cp[0-9]+)(-macosx_)([0-9]+_[0-9]+)([a-z0-9_]+)(\.whl)"
        regwhl = re.compile(patwhl)
        if not regwhl.match(original):
            print( "", file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to <rename wheel for anaconda3>" % myscript, file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            return 1
        else:
            ver = regwhl.match(original).groups()[7]
            new = original.replace( ver, "10_9" )
            os.rename( original, new )

    #-----------------------------------------------------
    # [6] Rename the "dist/" directory
    #-----------------------------------------------------
    os.rename( "dist", PymodDistDir[ModulePython] )
    return 0

#------------------------------------------------------------------------------
## To run the main Bash script "build.sh" with appropriate options
#
# @param[in] config     the build configuration
# @param[in] parameters the build parameters
#
# @return 0 on success; non-zero (1), otherwise
#------------------------------------------------------------------------------
def Run_Build_Command(config, parameters):
    DebugMode = config['DebugMode']
    ModuleQt  = config['ModuleQt']
    NoLibGit2 = config['NoLibGit2']
    ToolDebug = config['ToolDebug']
    if 100 not in ToolDebug: # default
        jump2pymod_wheel = False
    else:
        jump2pymod_wheel = True

    #-----------------------------------------------------------------
    # [1] Use the AddressSanitizer (ASan) in the debug build.
    #     This environment variable is tested in ../src/klayout.pri.
    #-----------------------------------------------------------------
    try:
        useAsan = os.environ['MAC_USE_ASAN']
    except KeyError:
        pass
    else:
        del os.environ['MAC_USE_ASAN']

    if DebugMode:
        os.environ['MAC_USE_ASAN'] = "1"

    #-----------------------------------------------------
    # [2] Set two environment variables to use libgit2
    #-----------------------------------------------------
    if not NoLibGit2:
        # Using MacPorts
        if ModuleQt.upper() in [ 'QT5MACPORTS', 'QT6MACPORTS' ]:
            addIncPath = "/opt/local/include"
            addLibPath = "/opt/local/lib"
        # Using Homebrew
        elif ModuleQt.upper() in [ 'QT5BREW', 'QT6BREW' ]:
            addIncPath = "%s/include" % DefaultHomebrewRoot  # defined in "build4mac_env.py"
            addLibPath = "%s/lib"     % DefaultHomebrewRoot  # -- ditto --
        # Using Anaconda3
        elif ModuleQt.upper() in [ 'QT5ANA3' ]:
            addIncPath = "/Applications/anaconda3/include"
            addLibPath = "/Applications/anaconda3/lib"
        else:
            addIncPath = ""
            addLibPath = ""

        # These environment variables are expanded on the fly in ../src/klayout.pri.
        if not addIncPath == "":
            os.environ['MAC_LIBGIT2_INC'] = "%s" % addIncPath
        else:
            os.environ['MAC_LIBGIT2_INC'] = "_invalid_MAC_LIBGIT2_INC_" # compile should fail

        if not addLibPath == "":
            os.environ['MAC_LIBGIT2_LIB'] = "%s" % addLibPath
        else:
            os.environ['MAC_LIBGIT2_LIB'] = "_invalid_MAC_LIBGIT2_LIB_" # link should fail

    if not jump2pymod_wheel:
        #-----------------------------------------------------
        # [3] Set parameters passed to the main Bash script
        #-----------------------------------------------------
        cmd_args = ""

        # (A) debug or release
        if parameters["debug_mode"]:
            mode      = "debug"
            cmd_args += "  -debug"
        else:
            mode      = "release"
            cmd_args += "  -release"

        # (C) Target directories and files
        MacBuildDirQAT = parameters['build'] + ".macQAT"

        # (D) Qt5 (MacPorts, Homebrew, Anaconda3) | Qt6 (MacPorts, Homebrew)
        cmd_args += " \\\n  -qmake %s" % parameters['qmake']
        cmd_args += " \\\n  -bin   %s" % parameters['bin']
        cmd_args += " \\\n  -build %s" % parameters['build']
        cmd_args += " \\\n  -rpath %s" % parameters['rpath']

        # (E) want Qt bindings with Ruby scripts?
        if parameters['no_qt_bindings']:
            cmd_args += " \\\n  -without-qtbinding"
        else:
            cmd_args += " \\\n  -with-qtbinding"

        # (F) want QtUiTools?
        if parameters['no_qt_uitools']:
            cmd_args += " \\\n  -without-qt-uitools"

        # (G) don't want to use libgit2?
        if NoLibGit2:
            cmd_args += " \\\n  -libgit2"

        # (H) options to `make` tool
        if 'make_options' in parameters:
            cmd_args += " \\\n  -option %s" % parameters['make_options']

        # (I) about Ruby
        if 'ruby' in parameters:
            cmd_args += " \\\n  -ruby   %s" % parameters['ruby']
            cmd_args += " \\\n  -rbinc  %s" % parameters['rbinc']
            cmd_args += " \\\n  -rblib  %s" % parameters['rblib']
            if 'rbinc2' in parameters:
                cmd_args += " \\\n  -rbinc2  %s" % parameters['rbinc2']
        else:
            cmd_args += " \\\n  -noruby"

        # (J) about Python
        if 'python' in parameters:
            cmd_args += " \\\n  -python %s" % parameters['python']
            cmd_args += " \\\n  -pyinc  %s" % parameters['pyinc']
            cmd_args += " \\\n  -pylib  %s" % parameters['pylib']
        else:
            cmd_args += " \\\n  -nopython"

        #-----------------------------------------------------------------------------------
        # [4] Make the consolidated command line
        #     "caffeinate" makes the CPU run at full speed even when the screen is locked.
        #-----------------------------------------------------------------------------------
        command  = "time"
        command += " \\\n  caffeinate -i %s" % parameters['build_cmd']
        command += cmd_args
        command += "  2>&1 | tee %s; \\\n" % parameters['logfile']
        command += "test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

        if parameters['check_cmd_only']:
            print(command)
            sys.exit(0)

        #-----------------------------------------------------
        # [5] Invoke the main Bash script; takes time:-)
        #-----------------------------------------------------
        myscript = os.path.basename(__file__)
        ret = subprocess.call( command, shell=True )
        if ret != 0:
            print( "", file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to build KLayout" % myscript, file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            return 1

        print( "", file=sys.stderr )
        print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
        print( "### <%s>: successfully built KLayout" % myscript, file=sys.stderr )
        print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
        print( "", file=sys.stderr )

        #------------------------------------------------------------------------
        # [6] Prepare "*.macQAT/" directory for the QATest.
        #     Binaries under "*.macQAT/" such as *.dylib will be touched later.
        #------------------------------------------------------------------------
        print( "### Preparing <%s>" % MacBuildDirQAT )
        if os.path.isdir( MacBuildDirQAT ):
            shutil.rmtree( MacBuildDirQAT )

        os.chdir( parameters['build'] )
        tarFile = "../macQATest.tar"
        tarCmdC = "tar cf %s ." % tarFile
        if subprocess.call( tarCmdC, shell=True ) != 0:
            print( "", file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to create <%s>" % (myscript, tarFile), file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            return 1

        os.chdir( "../" )
        os.mkdir( MacBuildDirQAT )
        os.chdir( MacBuildDirQAT )
        tarCmdX = "tar xf %s" % tarFile
        if subprocess.call( tarCmdX, shell=True ) != 0:
            print( "", file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "!!! <%s>: failed to unpack <%s>" % (myscript, tarFile), file=sys.stderr )
            print( "-------------------------------------------------------------", file=sys.stderr )
            print( "", file=sys.stderr )
            return 1

        os.remove( tarFile )
        os.chdir( "../" )
        shutil.copy2( "macbuild/macQAT.py", MacBuildDirQAT )
        print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
        print( "### <%s>: prepared the initial *.macQAT/" % myscript, file=sys.stderr )
        print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
        print( "", file=sys.stderr )

    #------------------------------------------------------------------------
    # [7] Build <pymod> for some predetermined environments on demand
    #------------------------------------------------------------------------
    BuildPymodWhl = parameters['BuildPymodWhl']
    if BuildPymodWhl:
        ret = Build_pymod_wheel(parameters)
        return ret
    else:
        return 0

#------------------------------------------------------------------------------
## For making a bundle (klayout.app), deploy built binaries and libraries
#  on which those binaries depend.
#
# Reference: "Deploying an Application on Mac OS X" of Qt Assistant.
#
# @param[in] config     the build configuration
# @param[in] parameters the build parameters
#
# @return 0 on success; non-zero on failure
#------------------------------------------------------------------------------
def Deploy_Binaries_For_Bundle(config, parameters):
    #-----------------------------------------------------
    # [0] Retrieve the configuration and build parameters
    #-----------------------------------------------------
    NonOSStdLang   = config['NonOSStdLang']
    DeploymentF    = config['DeploymentF']
    DeploymentP    = config['DeploymentP']
    MacPkgDir      = config['MacPkgDir']
    Version        = config['Version']
    DeployVerbose  = config['DeployVerbose']
    ModuleRuby     = config['ModuleRuby']
    ModulePython   = config['ModulePython']
    ToolDebug      = config['ToolDebug']
    EmbedQt        = config['EmbedQt']
    EmbedPython3   = config['EmbedPython3']

    BuildPymodWhl  = parameters['BuildPymodWhl']
    ProjectDir     = parameters['project_dir']
    MacBinDir      = parameters['bin']
    MacBuildDir    = parameters['build']
    MacBuildLog    = parameters['logfile']
    Platform       = parameters['Platform']
    QtLibRoot      = parameters['qt_lib_root']

    AbsMacPkgDir   = "%s/%s" % (ProjectDir, MacPkgDir)
    AbsMacBinDir   = "%s/%s" % (ProjectDir, MacBinDir)
    AbsMacBuildDir = "%s/%s" % (ProjectDir, MacBuildDir)
    AbsMacBuildLog = "%s/%s" % (ProjectDir, MacBuildLog)

    if BuildPymodWhl:
        try:
            PymodDistDir = parameters['pymod_dist']
            pymodDistDir = PymodDistDir[ModulePython] # [ 'dist-MP3-${ModuleQt}', 'dist-HB3-${ModuleQt}', 'dist-ana3-${ModuleQt}' ]
        except KeyError:
            pymodDistDir = ""
        else:
            pass
    else:
        pymodDistDir = ""

    print("")
    print( "##### Started deploying libraries and executables for <klayout.app> #####" )
    print( " [1] Checking the status of working directory ..." )
    #-------------------------------------------------------------
    # [1] Check the status of working directory
    #-------------------------------------------------------------
    os.chdir(ProjectDir)
    if not DeploymentF and not DeploymentP:
        return 1
    if DeploymentF and NonOSStdLang:
        print( "     WARNING!!! You chose <-y|--deploy> while using non-OS-standard script language.", file=sys.stderr )
        print( "         Consider using <-Y|--DEPLOY> instead", file=sys.stderr )
        #return 1
    if not os.path.isfile(MacBuildLog):
        print( "!!! Build log file <%s> is not present !!!" % MacBuildLog, file=sys.stderr )
        return 1
    if not os.path.isdir(MacBuildDir):
        print( "!!! Build directory <%s> is not present !!!" % MacBuildDir, file=sys.stderr )
        return 1
    if not os.path.isdir(MacBinDir):
        print( "!!! Binary directory <%s> is not present !!!" % MacBinDir, file=sys.stderr )
        return 1


    print( " [2] Creating a new empty directory <%s> for deployment ..." % MacPkgDir )
    #-------------------------------------------------------------
    # [2] Create a new empty directory for deploying binaries
    #-------------------------------------------------------------
    os.chdir(ProjectDir)
    if os.path.isfile(MacPkgDir):
        os.remove(MacPkgDir)
    if os.path.isdir(MacPkgDir):
        shutil.rmtree(MacPkgDir)
    os.mkdir(MacPkgDir)


    print( " [3] Creating the standard directory structure for 'klayout.app' bundle ..." )
    #--------------------------------------------------------------------------------------------------------------
    # [3] Create the directory skeleton for "klayout.app" bundle and command line buddy tools such as "strm2cif".
    #     They are stored in the directory structure below.
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
    #                             |              +-- 'db_plugins'  --sym.link--> ../MacOS/db_plugins/
    #                             |              +-- 'lay_plugins' --sym.link--> ../MacOS/lay_plugins/
    #                             |              +-- 'pymod'       --sym.link--> ../MacOS/pymod/
    #                             +-- MacOS/+
    #                             |         +-- 'klayout'
    #                             |         +-- db_plugins/
    #                             |         +-- lay_plugins/
    #                             |         +-- pymod/
    #                             |
    #                             +-- Buddy/+
    #                             |         +-- 'strm2cif'
    #                             |         +-- 'strm2dxf'
    #                             |         :
    #                             |         +-- 'strmxor'
    #                             |
    #                             +-- pymod-dist/+ (created only if *.whl is available)
    #                                            +-- klayout-0.27.8-cp38-cp38-macosx_10_9_x86_64.whl (example)(1)
    #
    #                                            (1) *.whl is install with 'pip3'
    #--------------------------------------------------------------------------------------------------------------
    targetDir0 = "%s/klayout.app/Contents" % AbsMacPkgDir
    targetDirR = targetDir0 + "/Resources"
    targetDirF = targetDir0 + "/Frameworks"
    targetDirM = targetDir0 + "/MacOS"
    targetDirB = targetDir0 + "/Buddy"
    targetDirP = targetDir0 + "/pymod-dist"
    os.makedirs(targetDirR)
    os.makedirs(targetDirF)
    os.makedirs(targetDirM)
    os.makedirs(targetDirB)
    if BuildPymodWhl and not pymodDistDir == "":
        os.makedirs(targetDirP)


    print( " [4-1] Copying KLayout's dynamic link libraries to 'Frameworks' ..." )
    #--------------------------------------------------------------------------------------------------------------
    # [4-1] Copy KLayout's dynamic link libraries to "Frameworks/" and create the library dependency dictionary.
    #       <<< Do this job in "Frameworks/" >>>
    #
    # Note:
    #     KLayout's dynamic link library is built as below:
    #       (1) libklayout_lay.0.26.1.dylib
    #       (2) libklayout_lay.0.26.dylib -> libklayout_lay.0.26.1.dylib
    #       (3) libklayout_lay.0.dylib -> libklayout_lay.0.26.1.dylib
    #       (4) libklayout_lay.dylib -> libklayout_lay.0.26.1.dylib
    #     where,
    #       (1) is an ordinary file with full version number 'major.minor.teeny'
    #           between "library_name."='libklayout_lay.' and '.dylib'
    #       (2) is a symbolic link with 'major.minor' version number
    #       (3) is a symbolic link with 'major' version number
    #       (4) is a symbolic link without version number number
    #
    #     The dynamic linker tries to find a library name in style (3) as shown
    #     in the example below.
    #
    # Example:
    #   MacBookPro2(1)$ otool -L klayout
    #   klayout:
    #       :
    #       :
    #     libklayout_tl.0.dylib (compatibility version 0.26.0, current version 0.26.1)
    #     libklayout_gsi.0.dylib (compatibility version 0.26.0, current version 0.26.1)
    #     libklayout_db.0.dylib (compatibility version 0.26.0, current version 0.26.1)
    #       :
    #--------------------------------------------------------------------------------------------------------------
    os.chdir(targetDirF)
    dynamicLinkLibs  = glob.glob( os.path.join( AbsMacBinDir, "*.dylib" ) )
    dependencyDic_1  = dict() # inter-library dependency dictionary
    pathDic_1        = dict() # paths to insert for each library
    for item in dynamicLinkLibs:
        if os.path.isfile(item) and not os.path.islink(item):
            #-------------------------------------------------------------------
            # (A) Copy an ordinary *.dylib file here by changing the name
            #     to style (3) and set its mode to 0755 (sanity check).
            #-------------------------------------------------------------------
            fullName = os.path.basename(item).split('.')
            # e.g. [ 'libklayout_lay', '0', '26', '1', 'dylib' ]
            nameStyle3 = fullName[0] + "." + fullName[1] + ".dylib"
            shutil.copy2( item, nameStyle3 )
            os.chmod( nameStyle3, 0o0755 )
            #-------------------------------------------------------------------
            # (B) Then get inter-library dependencies
            #     Note that will pull all dependencies and sort them out later
            #     dropping those which don't have a path entry
            #-------------------------------------------------------------------
            otoolCm   = "otool -L %s | grep dylib" % nameStyle3
            otoolOut  = os.popen( otoolCm ).read()
            dependDic = DecomposeLibraryDependency(otoolOut)
            dependencyDic_1.update(dependDic)
            #-------------------------------------------------------------------
            # (C) This library goes into Frameworks, hence record it's path there
            #-------------------------------------------------------------------
            pathDic_1[nameStyle3] = "@executable_path/../Frameworks/" + nameStyle3

    if 410 in ToolDebug:
        DumpDependencyDicPair( "In [4-1 410]:", dependencyDic_1, pathDic_1 )

    print( " [4-2] Copying the contents of the plugins to the place next to the application..." )
    os.chdir(ProjectDir)
    #-------------------------------------------------------------------------------
    # (A) Copy the contents of the plugin directories to a place next to
    #     the application binary
    #-------------------------------------------------------------------------------
    for piDir in [ "db_plugins", "lay_plugins" ]:
        os.makedirs( os.path.join( targetDirM, piDir ) )
        dynamicLinkLibs = glob.glob( os.path.join( MacBinDir, piDir, "*.dylib" ) )
        for item in dynamicLinkLibs:
            if os.path.isfile(item) and not os.path.islink(item):
                #-------------------------------------------------------------------
                # (B) Copy an ordinary *.dylib file here by changing the name
                #     to style (3) and set its mode to 0755 (sanity check).
                #-------------------------------------------------------------------
                fullName = os.path.basename(item).split('.')
                # e.g. [ 'libklayout_lay', '0', '26', '1', 'dylib' ]
                nameStyle3 = fullName[0] + "." + fullName[1] + ".dylib"
                destPath   = os.path.join( targetDirM, piDir, nameStyle3 )
                shutil.copy2( item, destPath )
                os.chmod( destPath, 0o0755 )
                #-------------------------------------------------------------------
                # (C) Then get inter-library dependencies
                #     Note that will pull all dependencies and sort them out later
                #     dropping those which don't have a path entry
                #-------------------------------------------------------------------
                otoolCm   = "otool -L %s | grep 'dylib'" % destPath
                otoolOut  = os.popen( otoolCm ).read()
                dependDic = DecomposeLibraryDependency(otoolOut)
                dependencyDic_1.update(dependDic)
                #-------------------------------------------------------------------
                # (D) This library goes into the plugin directory
                #-------------------------------------------------------------------
                pathDic_1[nameStyle3] = "@executable_path/" + piDir + "/" + nameStyle3

    if 420 in ToolDebug:
        DumpDependencyDicPair( "In [4-2 420]:", dependencyDic_1, pathDic_1 )

    print( " [4-3] Making symbolic links from 'Frameworks' to '../MacOS/[db_plugins | lay_plugins | pymod]'..." )
    #--------------------------------------------------------------------------------------------------------------
    # [4-3] Make symbolic links
    #        'db_plugins'  --slink--> ../MacOS/db_plugins/
    #        'lay_plugins' --slink--> ../MacOS/lay_plugins/
    #        'pymod'       --slink--> ../MacOS/pymod/
    #      under Frameworks/.
    #
    #     'db_plugins' is required for the command line Buddy tools.
    #        Ref. https://github.com/KLayout/klayout/issues/460#issuecomment-571803458
    #
    #             :
    #             +-- Frameworks/+
    #             |              +-- '*.framework'
    #             |              +-- '*.dylib'
    #             |              +-- 'db_plugins'  --sym.link--> ../MacOS/db_plugins/
    #             |              +-- 'lay_plugins' --sym.link--> ../MacOS/lay_plugins/
    #             |              +-- 'pymod'       --sym.link--> ../MacOS/pymod/
    #             +-- MacOS/+
    #             |         +-- 'klayout'
    #             |         +-- db_plugins/
    #             |         +-- lay_plugins/
    #             |         +-- pymod/
    #             :
    #--------------------------------------------------------------------------------------------------------------
    os.chdir(targetDirF)
    os.symlink( "../MacOS/db_plugins/",  "./db_plugins" )
    os.symlink( "../MacOS/lay_plugins/", "./lay_plugins" )
    os.symlink( "../MacOS/pymod/",       "./pymod" )

    print( " [4-4] Deeply copying 'pymod/*' to 'MacOS/pymod/*'..." )
    #--------------------------------------------------------------------------------------------------------------
    # [4-4] Deeply copy the 'pymod' directory's contents, if any
    #             :
    #             +-- Frameworks/+
    #             |              +-- '*.framework'
    #             |              +-- '*.dylib'
    #             |              +-- 'db_plugins'  --sym.link--> ../MacOS/db_plugins/
    #             |              +-- 'lay_plugins' --sym.link--> ../MacOS/lay_plugins/
    #             |              +-- 'pymod'       --sym.link--> ../MacOS/pymod/
    #             +-- MacOS/+
    #             |         +-- 'klayout'
    #             |         +-- db_plugins/
    #             |         +-- lay_plugins/
    #             |         +-- pymod/+
    #             |                   +-- klayout/+
    #             |                               +-- (*)
    #             |                   +-- pya/+
    #             |                           +-- __init__.py
    #             :
    # (*) Example
    #    -rwxr-xr-x  ...  QtCore.cpython-311-darwin.so --- (+)
    #    -rwxr-xr-x  ...  QtDesigner.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtGui.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtMultimedia.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtNetwork.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtPrintSupport.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtSql.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtSvg.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtUiTools.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtWidgets.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtXml.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  QtXmlPatterns.cpython-311-darwin.so
    #    -rwxr-xr-x  ...  __init__.py
    #    drwxr-xr-x  ...  __pycache__
    #    drwxr-xr-x  ...  db
    #    -rwxr-xr-x  ...  dbcore.cpython-311-darwin.so
    #    drwxr-xr-x  ...  lay
    #    -rwxr-xr-x  ...  laycore.cpython-311-darwin.so
    #    drwxr-xr-x  ...  lib
    #    -rwxr-xr-x  ...  libcore.cpython-311-darwin.so
    #    drwxr-xr-x  ...  pya
    #    -rwxr-xr-x  ...  pyacore.cpython-311-darwin.so
    #    drwxr-xr-x  ...  rdb
    #    -rwxr-xr-x  ...  rdbcore.cpython-311-darwin.so
    #    drwxr-xr-x  ...  tl
    #    -rwxr-xr-x  ...  tlcore.cpython-311-darwin.so
    #
    #--------------------------------------------------------------------------------------------------------------
    # (+) Example of the inter-library dependency of pymod's library
    #
    # % otool -L QtCore.cpython-311-darwin.so
    #
    # QtCore.cpython-311-darwin.so:
    #     libQtCore.0.dylib (compatibility version 0.28.0, current version 0.28.17)
    #     /opt/local/lib/libgit2.1.7.dylib (compatibility version 1.7.0, current version 1.7.2)
    #     /opt/local/lib/libz.1.dylib (compatibility version 1.0.0, current version 1.3.1)
    #     /usr/local/opt/python@3.11/Frameworks/Python.framework/Versions/3.11/Python (compatibility...)
    #     libklayout_tl.0.dylib (compatibility version 0.28.0, current version 0.28.17)
    #     libklayout_gsi.0.dylib (compatibility version 0.28.0, current version 0.28.17)
    #     libklayout_pya.0.dylib (compatibility version 0.28.0, current version 0.28.17)
    #     libklayout_QtCore.0.dylib (compatibility version 0.28.0, current version 0.28.17)
    #     libklayout_QtGui.0.dylib (compatibility version 0.28.0, current version 0.28.17)
    #     libklayout_QtWidgets.0.dylib (compatibility version 0.28.0, current version 0.28.17)
    #     /opt/local/libexec/qt5/lib/QtPrintSupport.framework/Versions/5/QtPrintSupport (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtDesigner.framework/Versions/5/QtDesigner (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtMultimediaWidgets.framework/Versions/5/QtMultimediaWidgets (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtSvg.framework/Versions/5/QtSvg (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtWidgets.framework/Versions/5/QtWidgets (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtMultimedia.framework/Versions/5/QtMultimedia (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtGui.framework/Versions/5/QtGui (compatibility...)
    #     /System/Library/Frameworks/AppKit.framework/Versions/C/AppKit (compatibility...)
    #     /System/Library/Frameworks/Metal.framework/Versions/A/Metal (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtXml.framework/Versions/5/QtXml (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtXmlPatterns.framework/Versions/5/QtXmlPatterns (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtNetwork.framework/Versions/5/QtNetwork (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtSql.framework/Versions/5/QtSql (compatibility...)
    #     /opt/local/libexec/qt5/lib/QtCore.framework/Versions/5/QtCore (compatibility...)
    #     /System/Library/Frameworks/DiskArbitration.framework/Versions/A/DiskArbitration (compatibility...)
    #     /System/Library/Frameworks/IOKit.framework/Versions/A/IOKit (compatibility...)
    #     /System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL (compatibility...)
    #     /System/Library/Frameworks/AGL.framework/Versions/A/AGL (compatibility...)
    #     /usr/lib/libc++.1.dylib (compatibility...)
    #     /usr/lib/libSystem.B.dylib (compatibility...)
    #--------------------------------------------------------------------------------------------------------------
    os.chdir(AbsMacBinDir)
    sourceDirKly    = "pymod/klayout"
    sourdeDirPya    = "pymod/pya"
    pymodDirInBin   = os.path.isdir(sourceDirKly) and os.path.isdir(sourdeDirPya)
    dependencyDic_2 = dict() # inter-library dependency dictionary
    pathDic_2       = dict() # paths to insert for each library
    dependencyDic_3 = dict() # inter-library dependency dictionary
    pathDic_3       = dict() # paths to insert for each library
    dependencyDic_4 = dict() # inter-library dependency dictionary
    pathDic_4       = dict() # paths to insert for each library

    if pymodDirInBin:
        targetDirKly = os.path.join(targetDirM, sourceDirKly)
        targetDirPya = os.path.join(targetDirM, sourdeDirPya)

        retK = Deeply_Copy_Dir( sourceDirKly, targetDirKly, excl_pat_list=["__pycache__"] )
        retP = Deeply_Copy_Dir( sourdeDirPya, targetDirPya, excl_pat_list=["__pycache__"] )
        if not (retK and retP):
            msg = "!!! Failed to deeply copy the 'pymod' directory's contents !!!"
            print(msg)
            return 1

        # <<< Do the remaining job in "MacOS/pymod/klayout/" >>>
        os.chdir(targetDirKly)
        #-------------------------------------------------------------------
        # (A) Prepare regular expressions for the library name-matching
        #-------------------------------------------------------------------
        # (1) KLayout's self libraries
        patSelf = r'^(lib.+[.]dylib)'
        regSelf = re.compile(patSelf)

        # (2) Auxiliary libraries such as 'libgit2.1.7.dylib'
        libAux_1 = "/opt/local/lib/"
        libAux_2 = "/usr/local/lib/"
        libAux_3 = "/opt/homebrew/lib/"
        libAux_4 = "/Applications/anaconda3/lib/"
        patAux   = r'^(%s|%s|%s|%s)(lib.+[.]dylib)' % (libAux_1, libAux_2, libAux_3, libAux_4)
        regAux   = re.compile(patAux)

        # (3) Qt frameworks
        #     QtLibRoot:
        #                MacPorts  ===> '/opt/local/libexec/qt5/lib'
        #                Homebrew  ===> '/usr/local/opt/qt@5/lib'
        #                Anaconda3 ===> '/Applications/anaconda3/lib' (not frameworks and won't be embedded)
        patQt = r'(%s/)(Qt.+[.]framework.+)' % QtLibRoot
        regQt = re.compile(patQt)

        # (4) Python frameworks (only for Homebrew) # in the case of Intel Mac...
        libPy3_1 = "%s/" % HBPython312FrameworkPath # /usr/local/opt/python@3.12/Frameworks/Python.framework/
        libPy3_2 = "%s/" % HBPython311FrameworkPath # /usr/local/opt/python@3.11/Frameworks/Python.framework/
        patPy3   = r'^(%s|%s)(.+)' % (libPy3_1, libPy3_2)
        regPy3   = re.compile(patPy3)

        #-------------------------------------------------------------------------------
        # (B) Copy the contents of the pymod/klayout/ directory to the place next to
        #     the main application executable
        #-------------------------------------------------------------------------------
        dynamicLinkLibs = glob.glob( os.path.join( targetDirM, sourceDirKly, "*.so" ) )
        for item in dynamicLinkLibs:
            if os.path.isfile(item) and not os.path.islink(item):
                #-------------------------------------------------------------------
                # (C) Copy an ordinary *.dylib file here by changing the name
                #     to style (3) and set its mode to 0755 (sanity check).
                #-------------------------------------------------------------------
                baseName = os.path.basename(item) # 'QtCore.cpython-311-darwin.so'
                fullName = os.path.basename(item).split('.') # [ 'QtCore', 'cpython-311-darwin', 'so' ]
                nameStyle3 = fullName[0] + "." + fullName[1] + ".so" # == fullName; no need to change!
                destPath = os.path.join( targetDirM, sourceDirKly, nameStyle3 )
                # shutil.copy2( item, destPath ) # already deeply copied!
                os.chmod( destPath, 0o0755 ) # just for confirmation

                #-------------------------------------------------------------------
                # (D) Then get inter-library dependencies
                #-------------------------------------------------------------------
                otoolCm   = "otool -L %s" % destPath
                otoolOut  = os.popen( otoolCm ).read()
                dependDic = DecomposeLibraryDependency(otoolOut)
                dicKey    = list(dependDic.keys())[0]
                dicVal    = dependDic[dicKey]
                dicValIdx = list( range(0, len(dicVal)) )

                #-------------------------------------------------------------------
                # (E) Dependencies on KLayout's self libraries (always)
                #     Populate 'dependencyDic_2' and 'pathDic_2'
                #-------------------------------------------------------------------
                if True:
                    dependLib_2 = dict()
                    for idx in dicValIdx:
                        fname = dicVal[idx]
                        if regSelf.match(fname):
                            if idx == 0:
                                dependLib_2[baseName] = baseName
                            else:
                                dependLib_2[fname] = fname

                    dependencyDic_2.update( {dicKey: dependLib_2} )
                    pathDic_2[nameStyle3] = "@executable_path/" + sourceDirKly + "/" + nameStyle3

                    for libname in dependLib_2.keys():
                        if libname == baseName:
                            continue
                        else:
                            pathDic_2[libname] = "@executable_path/../Frameworks/" + dependLib_2[libname]

                #-------------------------------------------------------------------
                # (F) Dependencies on Qt and auxiliary libraries (optional)
                #     Populate 'dependencyDic_3' and 'pathDic_3'
                #-------------------------------------------------------------------
                if EmbedQt:
                    dependLib_3 = dict()
                    for idx in dicValIdx:
                        fname = dicVal[idx]
                        if regAux.match(fname):
                            dependLib_3[fname] = regAux.match(fname).groups()[1]
                        elif regQt.match(fname):
                            dependLib_3[fname] = regQt.match(fname).groups()[1]

                    dependencyDic_3.update( {dicKey: dependLib_3} )
                    pathDic_3[nameStyle3] = "@executable_path/" + sourceDirKly + "/" + nameStyle3

                    for libname in dependLib_3.keys():
                        if libname == baseName:
                            continue
                        else:
                            pathDic_3[libname] = "@executable_path/../Frameworks/" + dependLib_3[libname]

                #-------------------------------------------------------------------
                # (G) Dependencies on Python framework (optional)
                #     Populate 'dependencyDic_4' and 'pathDic_4'
                #-------------------------------------------------------------------
                if EmbedPython3:
                    dependLib_4 = dict()
                    for idx in dicValIdx:
                        fname = dicVal[idx]
                        if regPy3.match(fname):
                            dependLib_4[fname] = "Python.framework/%s" % regPy3.match(fname).groups()[1]

                    dependencyDic_4.update( {dicKey: dependLib_4} )
                    pathDic_4[nameStyle3] = "@executable_path/" + sourceDirKly + "/" + nameStyle3

                    for libname in dependLib_4.keys():
                        if libname == baseName:
                            continue
                        else:
                            pathDic_4[libname] = "@executable_path/../Frameworks/" + dependLib_4[libname]

    if 441 in ToolDebug:
        DumpDependencyDicPair( "In [4-4 441]:", dependencyDic_2, pathDic_2 )

    if 442 in ToolDebug:
        DumpDependencyDicPair( "In [4-4 442]:", dependencyDic_3, pathDic_3 )

    if 443 in ToolDebug:
        DumpDependencyDicPair( "In [4-4 443]:", dependencyDic_4, pathDic_4 )

    print( " [5-1] Setting and changing the identification names among KLayout's libraries ..." )
    #-------------------------------------------------------------
    # [5-1] Set the identification names for KLayout's libraries
    #     and make the library aware of the locations of libraries
    #     on which it depends; that is, inter-library dependency
    #-------------------------------------------------------------
    os.chdir(targetDirF)
    ret = SetChangeIdentificationNameOfDyLib( dependencyDic_1, pathDic_1 )
    if not ret == 0:
        msg = "!!! Failed to set and change to new identification names with (dependencyDic_1, pathDic_1) !!!"
        print(msg)
        return 1

    print( " [5-2] Setting and changing the identification names among pymod's libraries ..." )
    #-------------------------------------------------------------
    # [5-2] Similarly for the pymod's libraries...
    #-------------------------------------------------------------
    if not targetDirKly == None:
        os.chdir(targetDirKly)
        if len(dependencyDic_2) > 0 and len(pathDic_2) > 0:
            ret = SetChangeIdentificationNameOfDyLib( dependencyDic_2, pathDic_2 )
            if not ret == 0:
                msg = "!!! Failed to set and change to new identification names with (dependencyDic_2, pathDic_2) !!!"
                print(msg)
                return 1

        if len(dependencyDic_3) > 0 and len(pathDic_3) > 0:
            ret = SetChangeIdentificationNameOfDyLib( dependencyDic_3, pathDic_3 )
            if not ret == 0:
                msg = "!!! Failed to set and change to new identification names with (dependencyDic_3, pathDic_3) !!!"
                print(msg)
                return 1

        if len(dependencyDic_4) > 0 and len(pathDic_4) > 0:
            ret = SetChangeIdentificationNameOfDyLib( dependencyDic_4, pathDic_4 )
            if not ret == 0:
                msg = "!!! Failed to set and change to new identification names with (dependencyDic_4, pathDic_4) !!!"
                print(msg)
                return 1

    print( " [6] Copying the built executables and resource files ..." )
    #-------------------------------------------------------------
    # [6] Copy some known files in source directories to
    #     relevant target directories
    #-------------------------------------------------------------
    os.chdir(ProjectDir)
    sourceDir0 = "%s/klayout.app/Contents" % MacBinDir
    sourceDir1 = sourceDir0 + "/MacOS"
    sourceDir2 = "%s/macbuild/Resources" % ProjectDir
    sourceDir3 = "%s" % MacBinDir

    # (A) the main components
    tmpfileM = ProjectDir + "/macbuild/Resources/Info.plist.template"
    keydicM  = { 'exe': 'klayout', 'icon': 'klayout.icns', 'bname': 'klayout', 'ver': Version }
    plistM   = GenerateInfoPlist( keydicM, tmpfileM )
    with open( targetDir0 + "/Info.plist", "w" ) as file:
        file.write(plistM)

    shutil.copy2( sourceDir0 + "/PkgInfo",      targetDir0 ) # this file is not mandatory
    shutil.copy2( sourceDir1 + "/klayout",      targetDirM )
    shutil.copy2( sourceDir2 + "/klayout.icns", targetDirR )

    os.chmod( targetDir0 + "/PkgInfo",      0o0644 )
    os.chmod( targetDir0 + "/Info.plist",   0o0644 )
    os.chmod( targetDirM + "/klayout",      0o0755 )
    os.chmod( targetDirR + "/klayout.icns", 0o0644 )

    # (B) the buddy command line tools
    buddies = glob.glob( sourceDir3 + "/strm*" )
    for item in buddies:
        shutil.copy2( item, targetDirB )
        buddy = os.path.basename(item)
        os.chmod( targetDirB + "/" + buddy, 0o0755 )

    # (C) the Pymod
    if BuildPymodWhl and not pymodDistDir == "":
        for item in glob.glob( pymodDistDir + "/*.whl" ):
            shutil.copy2( item,  targetDirP )

    print( " [7] Setting and changing the identification names of KLayout's libraries in each executable ..." )
    #------------------------------------------------------------------------------------
    # [7] Set and change the library identification name(s) of different executable(s)
    #------------------------------------------------------------------------------------
    os.chdir(ProjectDir)
    os.chdir(MacPkgDir)
    klayoutexec = "klayout.app/Contents/MacOS/klayout"
    ret = SetChangeLibIdentificationName( klayoutexec, "../Frameworks" )
    if not ret == 0:
        os.chdir(ProjectDir)
        msg = "!!! Failed to set/change library identification name for <%s> !!!"
        print( msg % klayoutexec, file=sys.stderr )
        return 1

    buddies     = glob.glob( "klayout.app/Contents/Buddy/strm*" )
    macdepQtOpt = ""
    for buddy in buddies:
        macdepQtOpt += " -executable=%s" % buddy
        ret = SetChangeLibIdentificationName( buddy, "../Frameworks" )
        if not ret == 0:
            os.chdir(ProjectDir)
            msg = "!!! Failed to set/change library identification name for <%s> !!!"
            print( msg % buddy, file=sys.stderr )
            return 1

    if DeploymentF:
        print( " [8] Finally, deploying Qt's Frameworks and auxiliary libraries ..." )
        #-------------------------------------------------------------
        # [8] Deploy Qt Frameworks
        #-------------------------------------------------------------
        verbose    = " -verbose=%d" % DeployVerbose
        app_bundle = "klayout.app"
        options    = macdepQtOpt + verbose
        deploytool = parameters['deploy_tool']

        # Without the following, the plugin cocoa would not be found properly.
        shutil.copy2( sourceDir2 + "/qt.conf", targetDirM )
        os.chmod( targetDirM + "/qt.conf", 0o0644 )

        os.chdir(ProjectDir)
        os.chdir(MacPkgDir)
        command = "%s %s %s" % ( deploytool, app_bundle, options )
        if subprocess.call( command, shell=True ) != 0:
            msg = "!!! Failed to deploy applications on OSX/macOS !!!"
            print( msg, file=sys.stderr )
            print("")
            os.chdir(ProjectDir)
            return 1

        #-----------------------------------------------------------------------------------------------
        # [9] Special deployment of Python3.11 from Homebrew
        #     To use Python3.11 from Homebrew on Sequoia...
        #       in "/usr/local/opt/python@3.11/lib/"
        #             Python.framework -> ../Frameworks/Python.framework/ <=== this symbolic was needed
        #             pkgconfig/
        #
        #     Use the "python3HB.py" tool to make different symbolic links [*] including the above one.
        #        Sequoia{kazzz-s} lib (1)% pwd
        #        /usr/local/opt/python@3.11/lib
        #        Sequoia{kazzz-s} lib (2)% ll
        #        total 0
        #        drwxr-xr-x  4 kazzz-s admin 128  9 21 23:03 .
        #        drwxr-xr-x 14 kazzz-s admin 448  9 21 18:33 ..
        #    [*] lrwxr-xr-x  1 kazzz-s admin  31  9 21 23:03 Python.framework -> ../Frameworks/Python.framework/
        #        drwxr-xr-x  4 kazzz-s admin 128  9  7 10:03 pkgconfig
        #
        #        Sequoia{kazzz-s} Python.framework (3)% pwd
        #        /usr/local/opt/python@3.11/Frameworks/Python.framework/Versions
        #        Sequoia{kazzz-s} Versions (4)% ll
        #        total 0
        #        drwxr-xr-x 4 kazzz-s admin 128  9 21 23:03 .
        #        drwxr-xr-x 6 kazzz-s admin 192  9 21 23:03 ..
        #        drwxr-xr-x 9 kazzz-s admin 288  9  7 10:03 3.11
        #    [*] lrwxr-xr-x 1 kazzz-s admin   5  9 21 23:03 Current -> 3.11/
        #
        #        Sequoia{kazzz-s} Python.framework (5)% pwd
        #        /usr/local/opt/python@3.11/Frameworks/Python.framework
        #        Sequoia{kazzz-s} Python.framework (6)% ll
        #        total 0
        #        drwxr-xr-x 6 kazzz-s admin 192  9 21 23:03 .
        #        drwxr-xr-x 3 kazzz-s admin  96  9  7 10:03 ..
        #    [*] lrwxr-xr-x 1 kazzz-s admin  25  9 21 23:03 Headers -> Versions/Current/Headers/
        #    [*] lrwxr-xr-x 1 kazzz-s admin  23  9 21 23:03 Python -> Versions/Current/Python
        #    [*] lrwxr-xr-x 1 kazzz-s admin  27  9 21 23:03 Resources -> Versions/Current/Resources/
        #        drwxr-xr-x 4 kazzz-s admin 128  9 21 23:03 Versions
        #-----------------------------------------------------------------------------------------------
        deploymentPython311HB  = (ModulePython == 'Python311Brew')
        deploymentPythonAutoHB = (ModulePython == 'PythonAutoBrew')
        if (deploymentPython311HB or deploymentPythonAutoHB) and NonOSStdLang:
            # from build4mac_util import WalkFrameworkPaths, PerformChanges
            # from build4mac_util import Change_Python_LibPath_RelativeToAbsolute, DumpDependencyDic
            if deploymentPython311HB:
                HBPythonFrameworkPath = HBPython311FrameworkPath
                pythonHBVer           = "3.11" # required to provide the legacy pip in HW-*.dmg
            elif deploymentPythonAutoHB:
                HBPythonFrameworkPath = HBPythonAutoFrameworkPath
                pythonHBVer           = HBPythonAutoVersion

            bundlePath          = AbsMacPkgDir + '/klayout.app'
            bundleExecPathAbs   = '%s/Contents/MacOS/' % bundlePath
            pythonFrameworkPath = '%s/Contents/Frameworks/Python.framework' % bundlePath
            testTarget          = '%s/Versions/%s/lib/python%s/test' % (pythonFrameworkPath, pythonHBVer, pythonHBVer)
            resourceTarget1     = '%s/Versions/%s/Resources' % (pythonFrameworkPath, pythonHBVer)
            resourceTarget2     = '%s/Resources' % pythonFrameworkPath
            binTarget           = '%s/Versions/%s/bin' % (pythonFrameworkPath, pythonHBVer)
            sitepackagesTarget  = '%s/Versions/%s/lib/python%s/site-packages' % (pythonFrameworkPath, pythonHBVer, pythonHBVer)
            sitepackagesSource  = '%s/Versions/%s/lib/python%s/site-packages' % (HBPythonFrameworkPath, pythonHBVer, pythonHBVer)

            print( "" )
            print( " [9] Optional deployment of Python from %s ..." % HBPythonFrameworkPath )
            print( "  [9.1] Copying Python Framework" )
            if 910 in ToolDebug:
                dbglevel = 910
            else:
                dbglevel = 0

            cmd01 = "rm -rf %s" % pythonFrameworkPath
            cmd02 = "rsync -a --safe-links %s/ %s" % (HBPythonFrameworkPath, pythonFrameworkPath)

            cmd03 = "rm -rf %s" % testTarget
            cmd04 = "rm -rf %s" % resourceTarget1
            cmd05 = "unlink %s" % resourceTarget2
            cmd06 = "rm -rf %s" % binTarget

            cmd07 = "mkdir %s" % sitepackagesTarget
            cmd08 = "cp -RL %s/{*distutils*,pip*,pkg_resources,setuptools*,wheel*} %s" % (sitepackagesSource, sitepackagesTarget)

            shell_commands = list()
            shell_commands.append(cmd01)
            shell_commands.append(cmd02)
            shell_commands.append(cmd03)
            shell_commands.append(cmd04)
            shell_commands.append(cmd05)
            shell_commands.append(cmd06)
            shell_commands.append(cmd07)
            shell_commands.append(cmd08)

            for command in shell_commands:
                if subprocess.call( command, shell=True ) != 0:
                    msg = "In [9.1], failed to execute command: %s"
                    print( msg % command, file=sys.stderr )
                    sys.exit(1)

            ret = Generate_Start_Console_Py( sourceDir2 + "/template-start-console.py",
                                             pythonHBVer,
                                             targetDirM + "/start-console.py" )
            if ret == False:
                print( "! Generate_Start_Console_Py() failed", file=sys.stderr )
                return 1

            shutil.copy2( sourceDir2 + "/klayout_console",  targetDirM )
            os.chmod( targetDirM + "/start-console.py", 0o0755 )
            os.chmod( targetDirM + "/klayout_console",  0o0755 )

            print( "  [9.2] Re-linking dylib dependencies inside Python.framework" )
            print( "   [9.2.1] Patching Python Framework" )
            if 921 in ToolDebug:
                dbglevel = 921
            else:
                dbglevel = 0
            Change_Python_LibPath_RelativeToAbsolute( pythonFrameworkPath, debug_level=dbglevel )
            depdict = WalkFrameworkPaths( pythonFrameworkPath, debug_level=dbglevel )
            DumpDependencyDic( "[9.2.1]", depdict, debug_level=dbglevel )
            appPythonFrameworkPath = '@executable_path/../Frameworks/Python.framework/'
            replacePairs = [ (HBPythonFrameworkPath, appPythonFrameworkPath, False) ]
            PerformChanges( depdict, replacePairs, bundleExecPathAbs, debug_level=dbglevel )

            print( "   [9.2.2] Patching 'Python' itself in Python Framework" )
            if 922 in ToolDebug:
                dbglevel = 922
            else:
                dbglevel = 0
            filterreg = r'\t+%s/(opt|Cellar)' % DefaultHomebrewRoot
            Patch_Python_In_PythonFramework( pythonFrameworkPath, filter_regex=filterreg, debug_level=dbglevel )

            print( "   [9.2.3] Patching %s/opt/ libs" % DefaultHomebrewRoot ) # eg. DefaultHomebrewRoot == "/usr/local"
            if 923 in ToolDebug:
                dbglevel = 923
            else:
                dbglevel = 0
            filterreg = r'\t+%s/(opt|Cellar)' % DefaultHomebrewRoot
            depdict   = WalkFrameworkPaths( pythonFrameworkPath, search_path_filter=filterreg, debug_level=dbglevel )
            DumpDependencyDic( "[9.2.3]", depdict, debug_level=dbglevel )
            usrLocalPath    = '%s/opt/' % DefaultHomebrewRoot
            appUsrLocalPath = '@executable_path/../Frameworks/'
            replacePairs    = [ (usrLocalPath, appUsrLocalPath, True) ]
            PerformChanges( depdict, replacePairs, bundleExecPathAbs, debug_level=dbglevel )

            #---------------------------------------------------------------------------------------------------
            # https://formulae.brew.sh/formula/python@3.9
            #   as of 2023-09-22, python@3.9 depends on:
            #     gdbm        1.23    GNU database manager
            #     mpdecimal   2.5.1   Library for decimal floating point arithmetic
            #     openssl@3   3.1.2   Cryptography and SSL/TLS Toolkit
            #     readline    8.2.1   Library for command-line editing
            #     sqlite      3.43.1  Command-line interface for SQLite
            #     xz          5.4.4   General-purpose data compression with high compression ratio
            #---------------------------------------------------------------------------------------------------
            # https://formulae.brew.sh/formula/python@3.11
            #   as of 2024-09-21, python@3.11 depends on:
            #     mpdecimal   4.0.0   Library for decimal floating point arithmetic
            #     openssl@3   3.3.2   Cryptography and SSL/TLS Toolkit
            #     sqlite      3.46.1  Command-line interface for SQLite
            #     xz          5.6.2   General-purpose data compression with high compression ratio
            #---------------------------------------------------------------------------------------------------
            print( "   [9.2.4] Patching [mpdecimal, openssl@3, sqlite, xz(, gdbm, readline)]" )
            if 924 in ToolDebug:
                dbglevel = 924
            else:
                dbglevel = 0
            usrLocalPath    = '%s/opt/' % DefaultHomebrewRoot
            appUsrLocalPath = '@executable_path/../Frameworks/'
            replacePairs    = [ (usrLocalPath, appUsrLocalPath, True) ]
            replacePairs.extend( [ (openssl_version, '@executable_path/../Frameworks/openssl@3', True)
                for openssl_version in glob.glob( '%s/Cellar/openssl@3/*' % DefaultHomebrewRoot ) ] )
            filterreg = r'\t+%s/(opt|Cellar)' % DefaultHomebrewRoot
            depdict   = WalkFrameworkPaths( [pythonFrameworkPath + '/../mpdecimal',
                                             pythonFrameworkPath + '/../openssl@3',
                                             pythonFrameworkPath + '/../sqlite',
                                             pythonFrameworkPath + '/../xz',
                                             pythonFrameworkPath + '/../gdbm',
                                             pythonFrameworkPath + '/../readline'],
                                             search_path_filter=filterreg,
                                             debug_level=dbglevel )

            DumpDependencyDic( "[9.2.4]", depdict, debug_level=dbglevel )
            PerformChanges( depdict, replacePairs, bundleExecPathAbs, debug_level=dbglevel )

            print( "  [9.3] Re-linking dylib dependencies for klayout" )
            if 931 in ToolDebug:
                dbglevel = 931
            else:
                dbglevel = 0
            klayoutPath = bundleExecPathAbs
            depdict     = WalkFrameworkPaths( klayoutPath, filter_regex=r'klayout$', debug_level=dbglevel )
            DumpDependencyDic( "[9.3.1]", depdict, debug_level=dbglevel )
            replacePairs = [ (HBPythonFrameworkPath, appPythonFrameworkPath, False) ]
            PerformChanges( depdict, replacePairs, bundleExecPathAbs, debug_level=dbglevel )

            if 932 in ToolDebug:
                dbglevel = 932
            else:
                dbglevel = 0
            libKlayoutPath = bundleExecPathAbs + '../Frameworks'
            depdict        = WalkFrameworkPaths( libKlayoutPath, filter_regex=r'libklayout', debug_level=dbglevel )
            DumpDependencyDic( "[9.3.2]", depdict, debug_level=dbglevel )
            replacePairs = [ (HBPythonFrameworkPath, appPythonFrameworkPath, False) ]
            PerformChanges( depdict, replacePairs, bundleExecPathAbs, debug_level=dbglevel )

            print( "  [9.4] Patching site.py and pip/" )
            if 940 in ToolDebug:
                dbglevel = 940
            else:
                dbglevel = 0
            site_module = "%s/Versions/%s/lib/python%s/site.py" % (pythonFrameworkPath, pythonHBVer, pythonHBVer)
            #-----------------------------------------------------------------------------------------
            # Rewrite the above <site.py> file.
            #     :
            #     # Prefixes for site-packages; add additional prefixes like /usr/local here
            #     PREFIXES = [sys.prefix, sys.exec_prefix]
            # (1) sys.real_prefix = sys.prefix <=== (1) add
            #     # Enable per user site-packages directory
            #     # set it to False to disable the feature or True to force the feature
            # (2) ENABLE_USER_SITE = False <=== (2) modify
            #     :
            #
            # This will fool pip into thinking it's inside a virtual environment
            # and install new packages to the correct site-packages.
            #-----------------------------------------------------------------------------------------
            # It looks like the technique of modifying <site.py> works when Python<=3.9 as follows.
            # However, it doesn't when 3.11<=Python.
            #
            # In </home/kazzz-s/GitWork/cpython-39/Lib/site.py> (Linux source code build)
            #    :
            #    # Prefixes for site-packages; add additional prefixes like /usr/local here
            #    PREFIXES = [sys.prefix, sys.exec_prefix]
            #    # Enable per user site-packages directory
            #    # set it to False to disable the feature or True to force the feature
            #    ENABLE_USER_SITE = None
            #    print( "### I have imported the modified <%s>" % __file__ ) <=== (3) added
            #    :
            #
            #  MyLinux{kazzz-s}(1)$ ./python
            #  ### I have imported the modified </home/kazzz-s/GitWork/cpython-39/Lib/site.py> ---(3)
            #  Python 3.9.18+ (heads/3.9:43a6e4fa49, Oct 27 2023, 12:51:17)
            #  [GCC 9.4.0] on linux
            #  Type "help", "copyright", "credits" or "license" for more information.
            #  >>>
            #-----------------------------------------------------------------------------------------
            with open(site_module, 'r') as site:
                buf = site.readlines()
            with open(site_module, 'w') as site:
                for line in buf:
                    if re.match("^PREFIXES", line) is not None:
                        line = line + "sys.real_prefix = sys.prefix\n" # --- (1)
                    # do not allow installation in the user folder.
                    if re.match("^ENABLE_USER_SITE", line) is not None:
                        line = "ENABLE_USER_SITE = False\n" # --- (2)
                    site.write(line)

            #-----------------------------------------------------------------------------------------
            # Typical usage of 'pip' after installation of the DMG package
            #
            # $ cd /Applications/klayout.app/Contents/MacOS/
            # $ ./start-console.py
            #
            # $ /Applications/klayout.app/Contents/MacOS/start-console.py
            # Warning: Populating font family aliases took 195 ms. Replace uses of missing font\
            # family "Monospace" with one that exists to avoid this cost.
            # Python 3.7.8 (default, Jul  4 2020, 10:17:17)
            # [Clang 11.0.3 (clang-1103.0.32.62)] on darwin
            # Type "help", "copyright", "credits" or "license" for more information.
            # (KLayout Python Console)
            # >>> import pip
            # >>> pip.main( ['install', 'pandas', 'scipy', 'matplotlib'] )
            # >>> quit()
            #
            # 'pandas' depends on many modules including 'numpy'. They are also installed.
            #-----------------------------------------------------------------------------------------
            pip_module = "%s/Versions/%s/lib/python%s/site-packages/pip/__init__.py" % \
                                     (pythonFrameworkPath, pythonHBVer, pythonHBVer)
            with open(pip_module, 'r') as pip:
                buf = pip.readlines()
            with open(pip_module, 'w') as pip:
                for line in buf:
                    # this will reject user's configuration of pip, forcing the isolated mode
                    line = re.sub("return isolated$", "return isolated or True", line)
                    pip.write(line)

            #----------------------------------------------------------------------------------------------------
            # Patch distutils/ in older versions of Python.
            # However, newer versions of Python deprecate the distutils.cfg file and the distutils module itself.
            #   Ref. https://github.com/Homebrew/homebrew-core/issues/76621
            #----------------------------------------------------------------------------------------------------
            """
            if deploymentPython39HB or (HBPythonAutoVersion == "3.9"): # Python == 3.9
                print( deploymentPython39HB, HBPythonAutoVersion )
                print( "  [9.5.1] Patching distutils/" )
                if 951 in ToolDebug:
                    dbglevel = 951
                else:
                    dbglevel = 0
                distutilsconfig = "%s/Versions/%s/lib/python%s/distutils/distutils.cfg" % \
                                                (pythonFrameworkPath, pythonHBVer, pythonHBVer)
                #-----------------------------------------------------------------------------------------
                # Rewrite the above <distutils.cfg> file in Homebrew python@3.9.18.
                #    [install]
                #    prefix=/usr/local <=== remove this line
                #    [build_ext]
                #    include_dirs=/usr/local/include:/usr/local/opt/openssl@3/include:\
                #                 /usr/local/opt/sqlite/include
                #    library_dirs=/usr/local/lib:/usr/local/opt/openssl@3/lib:/usr/local/opt/sqlite/lib
                #
                # This will cause all packages to be installed to sys.prefix
                #-----------------------------------------------------------------------------------------
                with open(distutilsconfig, 'r') as file:
                    buf = file.readlines()
                with open(distutilsconfig, 'w') as file:
                    for line in buf:
                        if re.match('prefix=', line) is not None:
                            continue
                        file.write(line)
            """
            if deploymentPython311HB or (HBPythonAutoVersion == "3.11"): # Python == 3.11
                # The above 'distutils.cfg' file does not exist in the distutils/ directory of Python 3.11.
                # Use a different technique.
                print( "  [9.5.2] In the Python3.11 environment, use an alternative method of patching distutils/" )
                print( "          See Contents/MacOS/start-console.py in /Applications/klayout.app/" )
            else: # other than ["3.11"]
                print( "!!! HW-dmg package assumes python@3.11" )
                print( "!!! 'distutils' has been deprecated in 3.12 <= Python" )
                print("")
                os.chdir(ProjectDir)
                return 1

        #-------------------------------------------------------------
        # [10] Special deployment of Ruby3.3 from Homebrew?
        #-------------------------------------------------------------
        deploymentRuby33HB = (ModuleRuby == 'Ruby34Brew')
        if deploymentRuby33HB and NonOSStdLang:

            print( "" )
            print( " [10] You have reached optional deployment of Ruby from %s ..." % HBRuby34Path )
            print( "   [!!!] Sorry, the deployed package will not work properly since deployment of" )
            print( "         Ruby3.3 from Homebrew is not yet supported." )
            print( "         Since you have Homebrew development environment, there two options:" )
            print( "           (1) Retry to make a package with '-Y|--DEPLOY' option." )
            print( "               This will not deploy any of Qt[5|6], Python, and Ruby from Homebrew." )
            print( "               Instead, the package will directly use those Frameworks and libraries" )
            print( "               in your Homebrew environment." )
            print( "           (2) Rebuild KLayout with '-r|--ruby <nil|Sys>' option depending on your preference." )
            print( "" )

    else:
        print( " [8] Skipped deploying Qt's Frameworks and optional Python/Ruby Frameworks..." )
    print( "##### Finished deploying the libraries and executables for <klayout.app> #####" )
    print("")
    os.chdir(ProjectDir)
    return 0

#------------------------------------------------------------------------------
## The main function
#------------------------------------------------------------------------------
def Main():
    pp = pprint.PrettyPrinter( indent=4, width=140 )
    config = Get_Default_Config()
    Parse_CLI_Args(config)

    #----------------------------------------------------------
    # [The main build stage]
    #----------------------------------------------------------
    parameters = Get_Build_Parameters(config)
    pp.pprint(parameters)

    if not config['DeploymentF'] and not config['DeploymentP']:
        ret = Run_Build_Command(config, parameters)
        pp.pprint(config)
        if not ret == 0:
            sys.exit(1)
    else:
        #----------------------------------------------------------
        # [The deployment stage]
        #   Deployment of dynamic link libraries, executables and
        #   resources to make the main "klayout.app" bundle
        #----------------------------------------------------------
        ret = Deploy_Binaries_For_Bundle(config, parameters)
        if not ret == 0:
            sys.exit(1)

#===================================================================================
if __name__ == "__main__":
    Main()

#---------------
# End of file
#---------------
