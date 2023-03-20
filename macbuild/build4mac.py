#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac.py"
#
#  The top Python script for building KLayout (http://www.klayout.de/index.php)
#  version 0.26.1 or later on different Apple Mac OSX platforms.
#===============================================================================
import sys
import os
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
    if platform.upper() in [ "VENTURA", "MONTEREY", "BIGSUR" ]: # with Xcode [13.1 .. ]
        myQt56    = "qt5brew"
        myRuby    = "hb32"
        myPython  = "hb39"
        moduleset = ('qt5Brew', 'HB32', 'HB39')
    else: # with Xcode [ .. 12.4]; 'sys' for Python has been restored in 0.28.3
        myQt56    = "qt5macports"
        myRuby    = "sys"
        myPython  = "sys"
        moduleset = ('qt5MP', 'Sys', 'Sys')

    usage  = "\n"
    usage += "---------------------------------------------------------------------------------------------------------\n"
    usage += "<< Usage of 'build4mac.py' >>\n"
    usage += "       for building KLayout 0.28.6 or later on different Apple macOS / Mac OSX platforms.\n"
    usage += "\n"
    usage += "$ [python] ./build4mac.py\n"
    usage += "   option & argument    : descriptions (refer to 'macbuild/build4mac_env.py' for details)| default value\n"
    usage += "   --------------------------------------------------------------------------------------+---------------\n"
    usage += "   [-q|--qt <type>]     : case-insensitive type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3',    | %s \n" % myQt56
    usage += "                        :                        'Qt6MacPorts', 'Qt6Brew']               | \n"
    usage += "                        :   Qt5MacPorts: use Qt5 from MacPorts                           | \n"
    usage += "                        :       Qt5Brew: use Qt5 from Homebrew                           | \n"
    usage += "                        :       Qt5Ana3: use Qt5 from Anaconda3                          | \n"
    usage += "                        :   Qt6MacPorts: use Qt6 from MacPorts (*)                       | \n"
    usage += "                        :       Qt6Brew: use Qt6 from Homebrew (*)                       | \n"
    usage += "                        :                        (*) migration to Qt6 is ongoing         | \n"
    usage += "   [-r|--ruby <type>]   : case-insensitive type=['nil', 'Sys', 'MP31', 'HB31', 'Ana3',   | %s \n" % myRuby
    usage += "                        :                        'MP32', HB32']                          | \n"
    usage += "                        :    nil: don't bind Ruby                                        | \n"
    usage += "                        :    Sys: use OS-bundled Ruby [2.0 - 2.6] depending on OS        | \n"
    usage += "                        :   MP31: use Ruby 3.1 from MacPorts                             | \n"
    usage += "                        :   HB31: use Ruby 3.1 from Homebrew                             | \n"
    usage += "                        :   Ana3: use Ruby 3.1 from Anaconda3                            | \n"
    usage += "                        :   MP32: use Ruby 3.2 from MacPorts                             | \n"
    usage += "                        :   HB32: use Ruby 3.2 from Homebrew                             | \n"
    usage += "   [-p|--python <type>] : case-insensitive type=['nil',  'Sys', 'MP38', 'HB38', 'Ana3',  | %s \n" % myPython
    usage += "                        :                        'MP39', HB39', 'HBAuto']                | \n"
    usage += "                        :    nil: don't bind Python                                      | \n"
    usage += "                        :    Sys: use OS-bundled Python 2.7 up to Catalina               | \n"
    usage += "                        :   MP38: use Python 3.8 from MacPorts                           | \n"
    usage += "                        :   HB38: use Python 3.8 from Homebrew                           | \n"
    usage += "                        :   Ana3: use Python 3.9 from Anaconda3                          | \n"
    usage += "                        :   MP39: use Python 3.9 from MacPorts                           | \n"
    usage += "                        :   HB39: use Python 3.9 from Homebrew                           | \n"
    usage += "                        : HBAuto: use the latest Python 3.x auto-detected from Homebrew  | \n"
    usage += "   [-P|--buildPymod]    : build and deploy Pymod (*.whl and *.egg) for LW-*.dmg          | disabled\n"
    usage += "   [-n|--noqtbinding]   : don't create Qt bindings for ruby scripts                      | disabled\n"
    usage += "   [-u|--noqtuitools]   : don't include uitools in Qt binding                            | disabled\n"
    usage += "   [-m|--make <option>] : option passed to 'make'                                        | '--jobs=4'\n"
    usage += "   [-d|--debug]         : enable debug mode build                                        | disabled\n"
    usage += "   [-c|--checkcom]      : check command-line and exit without building                   | disabled\n"
    usage += "   [-y|--deploy]        : deploy executables and dylibs, including Qt's Frameworks       | disabled\n"
    usage += "   [-Y|--DEPLOY]        : deploy executables and dylibs for those who built KLayout      | disabled\n"
    usage += "                        : from the source code and use the tools in the same machine     | \n"
    usage += "                        : ! After confirmation of the successful build of 'klayout.app', | \n"
    usage += "                        :   rerun this script with BOTH:                                 | \n"
    usage += "                        :     1) the same options used for building AND                  | \n"
    usage += "                        :     2) <-y|--deploy> OR <-Y|--DEPLOY>                          | \n"
    usage += "                        :   optionally with [-v|--verbose <0-3>]                         | \n"
    usage += "   [-v|--verbose <0-3>] : verbose level of `macdeployqt' (effective with -y only)        | 1\n"
    usage += "                        : 0 = no output, 1 = error/warning (default),                    | \n"
    usage += "                        : 2 = normal,    3 = debug                                       | \n"
    usage += "   [-?|--?]             : print this usage and exit; in zsh, quote like '-?' or '--?'    | disabled\n"
    usage += "-----------------------------------------------------------------------------------------+---------------\n"
    return (usage, moduleset)

#-------------------------------------------------------------------------------
## To get the default configurations
#
# @return a dictionary containing the default configuration for the macOS build
#-------------------------------------------------------------------------------
def Get_Default_Config():
    ProjectDir = os.getcwd()
    BuildBash  = "./build.sh"
    (System, Node, Release, MacVersion, Machine, Processor) = platform.uname()

    if not System == "Darwin":
        print("")
        print( "!!! Sorry. Your system <%s> looks like non-Mac" % System, file=sys.stderr )
        print( GenerateUsage("")[0] )
        sys.exit(1)

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
        print("")
        print( "!!! Sorry. Unsupported major OS release <%d>" % release, file=sys.stderr )
        print( GenerateUsage("")[0] )
        sys.exit(1)

    if not Machine == "x86_64":
        if Machine == "arm64" and Platform in ["Ventura", "Monterey", "BigSur"]: # with an Apple Silicon Chip
            print("")
            print( "### Your Mac equips an Apple Silicon Chip ###" )
            print("")
        else:
            print("")
            print( "!!! Sorry. Only x86_64/arm64 architecture machine is supported but found <%s>" % Machine, file=sys.stderr )
            print( GenerateUsage("")[0] )
            sys.exit(1)

    # Set the OS-wise usage and module set
    Usage, ModuleSet = GenerateUsage(Platform)

    # Set the default modules
    if   Platform == "Ventura":
        ModuleQt     = "Qt5Brew"
        ModuleRuby   = "Ruby32Brew"
        ModulePython = "Python39Brew"
    elif Platform == "Monterey":
        ModuleQt     = "Qt5Brew"
        ModuleRuby   = "Ruby32Brew"
        ModulePython = "Python39Brew"
    elif Platform == "BigSur":
        ModuleQt     = "Qt5Brew"
        ModuleRuby   = "Ruby32Brew"
        ModulePython = "Python39Brew"
    elif Platform == "Catalina":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "RubyCatalina"
        ModulePython = "PythonCatalina"
    elif Platform == "Mojave":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "RubyMojave"
        ModulePython = "PythonMojave"
    elif Platform == "HighSierra":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "RubyHighSierra"
        ModulePython = "PythonHighSierra"
    elif Platform == "Sierra":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "RubySierra"
        ModulePython = "PythonSierra"
    elif Platform == "ElCapitan":
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "RubyElCapitan"
        ModulePython = "PythonElCapitan"
    else:
        ModuleQt     = "Qt5MacPorts"
        ModuleRuby   = "nil"
        ModulePython = "nil"

    BuildPymod    = False
    NonOSStdLang  = False
    NoQtBindings  = False
    NoQtUiTools   = False
    MakeOptions   = "--jobs=4"
    DebugMode     = False
    CheckComOnly  = False
    DeploymentF   = False
    DeploymentP   = False
    PackagePrefix = ""
    DeployVerbose = 1
    Version       = GetKLayoutVersionFrom( "./version.sh" )

    config = dict()
    config['ProjectDir']    = ProjectDir        # project directory where "build.sh" exists
    config['Usage']         = Usage             # string on usage
    config['BuildBash']     = BuildBash         # the main build Bash script
    config['Platform']      = Platform          # platform
    config['ModuleQt']      = ModuleQt          # Qt module to be used
    config['ModuleRuby']    = ModuleRuby        # Ruby module to be used
    config['ModulePython']  = ModulePython      # Python module to be used
    config['BuildPymod']    = BuildPymod        # True to build and deploy "Pymod"
    config['NonOSStdLang']  = NonOSStdLang      # True if non-OS-standard language is chosen
    config['NoQtBindings']  = NoQtBindings      # True if not creating Qt bindings for Ruby scripts
    config['NoQtUiTools']   = NoQtUiTools       # True if not to include QtUiTools in Qt binding
    config['MakeOptions']   = MakeOptions       # options passed to `make`
    config['DebugMode']     = DebugMode         # True if debug mode build
    config['CheckComOnly']  = CheckComOnly      # True if only for checking the command line parameters to "build.sh"
    config['DeploymentF']   = DeploymentF       # True if fully (including Qt's Frameworks) deploy the binaries for bundles
    config['DeploymentP']   = DeploymentP       # True if partially deploy the binaries excluding Qt's Frameworks
    config['PackagePrefix'] = PackagePrefix     # the package prefix: 'ST-', 'LW-', 'HW-', or 'EX-'
    config['DeployVerbose'] = DeployVerbose     # -verbose=<0-3> level passed to 'macdeployqt' tool
    config['Version']       = Version           # KLayout's version
    config['ModuleSet']     = ModuleSet         # (Qt, Ruby, Python)-tuple
    # auxiliary variables on platform
    config['System']        = System            # 6-tuple from platform.uname()
    config['Node']          = Node              # - do -
    config['Release']       = Release           # - do -
    config['MacVersion']    = MacVersion        # - do -
    config['Machine']       = Machine           # - do -
    config['Processor']     = Processor         # - do -
    return config

#------------------------------------------------------------------------------
## To apply a workaround patch to "./src/klayout.pri" to work with Ruby 3.x.
#
# @param[in] config     dictionary containing the default configuration
#
# @return void
#------------------------------------------------------------------------------
def ApplyPatch2KLayoutQtPri4Ruby3(config):
    #----------------------------------------------------------------
    # [1] Check if the previous patch exists
    #----------------------------------------------------------------
    priMaster   = "./src/klayout.pri"
    priOriginal = "./src/klayout.pri.org"
    if os.path.exists(priOriginal):
        shutil.copy2( priOriginal, priMaster )
        os.remove( priOriginal )

    #----------------------------------------------------------------
    # [2] Not using Ruby?
    #----------------------------------------------------------------
    ModuleRuby = config['ModuleRuby']
    if ModuleRuby == 'nil':
        return;

    #----------------------------------------------------------------
    # [3] Get the Ruby version code as done in "build.sh"
    #----------------------------------------------------------------
    rubyExe  = RubyDictionary[ModuleRuby]['exe']
    oneline  = "puts (RbConfig::CONFIG['MAJOR'] || 0).to_i*10000+(RbConfig::CONFIG['MINOR'] || 0).to_i*100+(RbConfig::CONFIG['TEENY'] || 0).to_i"
    command  = [ '%s' % rubyExe, '-rrbconfig', '-e', '%s' % oneline ]
    verCode  = subprocess.check_output( command, encoding='utf-8' ).strip() # like 3.1.2 => "30102"
    verInt   = int(verCode)
    verMajor = verInt // 10000
    verMinor = (verInt - verMajor * 10000) // 100
    verTeeny = (verInt - verMajor * 10000) - (verMinor * 100)
    # print( verMajor, verMinor, verTeeny )
    # quit()
    if verMajor < 3:
        return;

    #-----------------------------------------------------------------------------------------------
    # [4] The two buggy Apple compilers below flag errors like:
    #
    #     /Applications/anaconda3/include/ruby-3.1.0/ruby/internal/intern/vm.h:383:1: error: \
    #     '__declspec' attributes are not enabled; use '-fdeclspec' or '-fms-extensions' to \
    #     enable support for __declspec attributes RBIMPL_ATTR_NORETURN()
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #   Problematic in <Catalina> with
    #     Apple clang version 12.0.0 (clang-1200.0.32.29)
    #     Target: x86_64-apple-darwin19.6.0
    #     Thread model: posix
    #
    #   Problematic in <Big Sur> with
    #     Apple clang version 13.0.0 (clang-1300.0.29.30)
    #     Target: x86_64-apple-darwin20.6.0
    #     Thread model: posix
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #   Non-problematic in <Monterey> with
    #     Apple clang version 13.1.6 (clang-1316.0.21.2.5)
    #     Target: x86_64-apple-darwin21.6.0
    #     Thread model: posix
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #   Refer to https://github.com/nginx/unit/issues/653
    #            https://github.com/nginx/unit/issues/653#issuecomment-1062129080
    #
    #   Pass "-fdeclspec" option to the QMAKE_CXXFLAGS macro via the "./src/klayout.pri" file like:
    """
    # <build4mac.py> applied this patch for Mac to work with Ruby 3.x
    mac {
        QMAKE_CXXFLAGS += -fdeclspec
    }
    # <build4mac.py> applied this patch for Mac to work with Ruby 3.x
    """
    #-----------------------------------------------------------------------------------------------
    #----------------------------------------------------------------
    # (A) Check Platform
    #----------------------------------------------------------------
    Platform = config['Platform']
    if Platform in [ "Ventura", "Monterey" ]:
        return
    elif Platform in [ "BigSur", "Catalina" ]: # take care
        pass
    else:
        return # the results are not tested and unknown

    #----------------------------------------------------------------
    # (B) Check ./src/klayout.pri and apply the patch if necessary
    #----------------------------------------------------------------
    keystring = "<build4mac.py> applied this patch for Mac to work with Ruby 3.x"
    patPatch  = r"(^#)([ ]*)(%s)([ ]*$)" % keystring
    regPatch  = re.compile(patPatch)
    foundKey1 = False
    foundKey2 = False

    with codecs.open( priMaster, "r", "utf-8" ) as file:
        allLines = file.readlines()
        file.close()
        for line in allLines:
            if regPatch.match( line.strip() ):
                if not foundKey1:
                    foundKey1 = True
                    continue
                elif not foundKey2:
                    foundKey2 = True
                    break
    if foundKey1 and foundKey2:
        return

    shutil.copy2( priMaster, priOriginal )
    with codecs.open( priMaster, "a", "utf-8" ) as file:
        file.write( "# %s\n" % keystring )
        file.write( "mac {\n" )
        file.write( "    QMAKE_CXXFLAGS += -fdeclspec\n" )
        file.write( "}\n" )
        file.write( "# %s\n" % keystring )
    return

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
    BuildPymod    = config['BuildPymod']
    NonOSStdLang  = config['NonOSStdLang']
    NoQtBindings  = config['NoQtBindings']
    NoQtUiTools   = config['NoQtUiTools']
    MakeOptions   = config['MakeOptions']
    DebugMode     = config['DebugMode']
    CheckComOnly  = config['CheckComOnly']
    DeploymentF   = config['DeploymentF']
    DeploymentP   = config['DeploymentP']
    PackagePrefix = config['PackagePrefix']
    DeployVerbose = config['DeployVerbose']
    ModuleSet     = config['ModuleSet']

    #-----------------------------------------------------
    # [2] Parse the CLI arguments
    #-----------------------------------------------------
    p = optparse.OptionParser(usage=Usage)
    p.add_option( '-q', '--qt',
                    dest='type_qt',
                    help="Qt type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3', 'Qt6MacPorts', 'Qt6Brew']" )

    p.add_option( '-r', '--ruby',
                    dest='type_ruby',
                    help="Ruby type=['nil', 'Sys', 'MP31', 'HB31', 'Ana3', 'MP32', 'HB32']" )

    p.add_option( '-p', '--python',
                    dest='type_python',
                    help="Python type=['nil', 'Sys', 'MP38', 'HB38', 'Ana3', 'MP39', 'HB39', 'HBAuto']" )

    p.add_option( '-P', '--buildPymod',
                    action='store_true',
                    dest='build_pymod',
                    default=False,
                    help="build and deploy <Pymod> (disabled)" )

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

    p.add_option( '-?', '--??',
                    action='store_true',
                    dest='checkusage',
                    default=False,
                    help='check usage' )

    if Platform.upper() in [ "VENTURA", "MONTEREY", "BIGSUR" ]: # with Xcode [13.1 .. ]
        p.set_defaults( type_qt        = "qt5brew",
                        type_ruby      = "hb32",
                        type_python    = "hb39",
                        build_pymod    = False,
                        no_qt_binding  = False,
                        no_qt_uitools  = False,
                        make_option    = "--jobs=4",
                        debug_build    = False,
                        check_command  = False,
                        deploy_full    = False,
                        deploy_partial = False,
                        deploy_verbose = "1",
                        checkusage     = False )
    else: # with Xcode [ .. 12.4]
        p.set_defaults( type_qt        = "qt5macports",
                        type_ruby      = "sys",
                        type_python    = "sys",
                        build_pymod    = False,
                        no_qt_binding  = False,
                        no_qt_uitools  = False,
                        make_option    = "--jobs=4",
                        debug_build    = False,
                        check_command  = False,
                        deploy_full    = False,
                        deploy_partial = False,
                        deploy_verbose = "1",
                        checkusage     = False )

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

    # By default, OS-standard (-bundled) script languages (Ruby and Python) are used
    NonOSStdLang = False

    # (B) Determine the Ruby type
    candidates         = dict()
    candidates['NIL']  = 'nil'
    candidates['SYS']  = 'Sys'
    candidates['MP31'] = 'MP31'
    candidates['HB31'] = 'HB31'
    candidates['ANA3'] = 'Ana3'
    candidates['MP32'] = 'MP32'
    candidates['HB32'] = 'HB32'
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
            choiceRuby = "Sys"
            if Platform == "Ventura":
                ModuleRuby = 'RubyVentura'
            elif Platform == "Monterey":
                ModuleRuby = 'RubyMonterey'
            elif Platform == "BigSur":
                ModuleRuby = 'RubyBigSur'
            elif Platform == "Catalina":
                ModuleRuby = 'RubyCatalina'
            elif Platform == "Mojave":
                ModuleRuby = 'RubyMojave'
            elif Platform == "HighSierra":
                ModuleRuby = 'RubyHighSierra'
            elif Platform == "Sierra":
                ModuleRuby = 'RubySierra'
            elif Platform == "ElCapitan":
                ModuleRuby = 'RubyElCapitan'
        elif choiceRuby == "MP31":
            ModuleRuby   = 'Ruby31MacPorts'
            NonOSStdLang = True
        elif choiceRuby == "HB31":
            ModuleRuby   = 'Ruby31Brew'
            NonOSStdLang = True
        elif choiceRuby == "Ana3":
            ModuleRuby   = 'RubyAnaconda3'
            NonOSStdLang = True
        elif choiceRuby == "MP32":
            ModuleRuby   = 'Ruby32MacPorts'
            NonOSStdLang = True
        elif choiceRuby == "HB32":
            ModuleRuby   = 'Ruby32Brew'
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
    candidates['SYS']    = 'Sys'   # has been restored in 0.28.3
    candidates['MP38']   = 'MP38'
    candidates['HB38']   = 'HB38'
    candidates['ANA3']   = 'Ana3'
    candidates['MP39']   = 'MP39'
    candidates['HB39']   = 'HB39'
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
        elif choicePython == "Sys":
            if Platform in [ "Ventura", "Monterey", "BigSur" ]:
                raise Exception( "! Cannot choose the 'sys' Python on <%s>" % Platform )
            elif Platform == "Catalina":
                ModulePython = 'PythonCatalina'
            elif Platform == "Mojave":
                ModulePython = 'PythonMojave'
            elif Platform == "HighSierra":
                ModulePython = 'PythonHighSierra'
            elif Platform == "Sierra":
                ModulePython = 'PythonSierra'
            elif Platform == "ElCapitan":
                ModulePython = 'PythonElCapitan'
        elif choicePython == "MP38":
            ModulePython = 'Python38MacPorts'
            NonOSStdLang = True
        elif choicePython == "HB38":
            ModulePython = 'Python38Brew'
            NonOSStdLang = True
        elif choicePython == "Ana3":
            ModulePython = 'PythonAnaconda3'
            NonOSStdLang = True
        elif choicePython == "MP39":
            ModulePython = 'Python39MacPorts'
        elif choicePython == "HB39":
            ModulePython = 'Python39Brew'
            NonOSStdLang = True
        elif choicePython == "HBAuto":
            ModulePython = 'PythonAutoBrew'
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
    BuildPymod   = opt.build_pymod
    NoQtBindings = opt.no_qt_binding
    NoQtUiTools  = opt.no_qt_uitools
    MakeOptions  = opt.make_option
    DebugMode    = opt.debug_build
    CheckComOnly = opt.check_command
    DeploymentF  = opt.deploy_full
    DeploymentP  = opt.deploy_partial

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

    if not DeploymentF and not DeploymentP:
        target  = "%s %s %s" % (Platform, Release, Machine)
        modules = "Qt=%s, Ruby=%s, Python=%s" % (ModuleQt, ModuleRuby, ModulePython)
        if BuildPymod:
            pymodbuild = "enabled"
        else:
            pymodbuild = "disabled"
        message = "### You are going to build KLayout\n    for  <%s>\n    with <%s>\n    with Pymod <%s>...\n"
        print("")
        print( message % (target, modules, pymodbuild) )
    else:
        message = "### You are going to make "
        if DeploymentP:
            PackagePrefix = "LW-"
            if not BuildPymod:
                message += "a lightweight (LW-) package excluding Qt5, Ruby, and Python..."
            else:
                message += "a lightweight (LW-) package with Pymod excluding Qt5, Ruby, and Python..."
        elif DeploymentF:
            if (ModuleRuby in RubySys) and (ModulePython in PythonSys):
                PackagePrefix = "ST-"
                message      += "a standard (ST-) package including Qt[5|6] and using OS-bundled Ruby and Python..."
            elif ModulePython in ['Python38Brew', 'Python39Brew', 'PythonAutoBrew']:
                PackagePrefix = "HW-"
                message      += "a heavyweight (HW-) package including Qt[5|6] and Python3.8~ from Homebrew..."
            else:
                PackagePrefix = "EX-"
                message      += "a package with exceptional (EX-) combinations of different modules..."
        print( "" )
        print( message )
        print( "" )
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
    config['BuildPymod']    = BuildPymod
    config['NonOSStdLang']  = NonOSStdLang
    config['NoQtBindings']  = NoQtBindings
    config['NoQtUiTools']   = NoQtUiTools
    config['MakeOptions']   = MakeOptions
    config['DebugMode']     = DebugMode
    config['CheckComOnly']  = CheckComOnly
    config['DeploymentF']   = DeploymentF
    config['DeploymentP']   = DeploymentP
    config['PackagePrefix'] = PackagePrefix
    config['DeployVerbose'] = DeployVerbose
    config['ModuleSet']     = ModuleSet

    if CheckComOnly:
        pp = pprint.PrettyPrinter( indent=4, width=140 )
        parameters = Get_Build_Parameters(config)
        Build_pymod(parameters)
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
    BuildPymod    = config['BuildPymod']
    ModuleSet     = config['ModuleSet']
    NoQtBindings  = config['NoQtBindings']
    NoQtUiTools   = config['NoQtUiTools']
    MakeOptions   = config['MakeOptions']
    DebugMode     = config['DebugMode']
    CheckComOnly  = config['CheckComOnly']
    DeploymentF   = config['DeploymentF']
    DeploymentP   = config['DeploymentP']
    PackagePrefix = config['PackagePrefix']

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
    parameters['logfile'] = MacBuildLog

    # (D) Qt5|6
    if ModuleQt == 'Qt5MacPorts':
        parameters['qmake']       = Qt5MacPorts['qmake']
        parameters['deploy_tool'] = Qt5MacPorts['deploy']
    elif ModuleQt == 'Qt5Brew':
        parameters['qmake']       = Qt5Brew['qmake']
        parameters['deploy_tool'] = Qt5Brew['deploy']
    elif ModuleQt == 'Qt5Ana3':
        parameters['qmake']       = Qt5Ana3['qmake']
        parameters['deploy_tool'] = Qt5Ana3['deploy']
    elif ModuleQt == 'Qt6MacPorts':
        parameters['qmake']       = Qt6MacPorts['qmake']
        parameters['deploy_tool'] = Qt6MacPorts['deploy']
    elif ModuleQt == 'Qt6Brew':
        parameters['qmake']       = Qt6Brew['qmake']
        parameters['deploy_tool'] = Qt6Brew['deploy']

    parameters['bin']   = MacBinDir
    parameters['build'] = MacBuildDir
    parameters['rpath'] = "@executable_path/../Frameworks"

    # (E) want Qt bindings with Ruby scripts?
    parameters['no_qt_bindings'] = NoQtBindings

    # (F) want QtUiTools?
    parameters['no_qt_uitools'] = NoQtUiTools

    # (G) options to `make` tool
    if not MakeOptions == "":
        parameters['make_options'] = MakeOptions
        try:
            jobopt, number = MakeOptions.split('=')  # like '--jobs=4' ?
            pnum = int(number)
        except Exception:
            parameters['num_parallel'] = 4  # default
        else:
            parameters['num_parallel'] = pnum

    # (H) about Ruby
    if ModuleRuby != "nil":
        ApplyPatch2KLayoutQtPri4Ruby3( config )
        parameters['ruby']  = RubyDictionary[ModuleRuby]['exe']
        parameters['rbinc'] = RubyDictionary[ModuleRuby]['inc']
        parameters['rblib'] = RubyDictionary[ModuleRuby]['lib']
        if 'inc2' in RubyDictionary[ModuleRuby]:
            parameters['rbinc2'] = RubyDictionary[ModuleRuby]['inc2']

    # (I) about Python
    if ModulePython != "nil":
        parameters['python'] = PythonDictionary[ModulePython]['exe']
        parameters['pyinc']  = PythonDictionary[ModulePython]['inc']
        parameters['pylib']  = PythonDictionary[ModulePython]['lib']

    config['MacPkgDir']      = MacPkgDir        # relative path to package directory
    config['MacBinDir']      = MacBinDir        # relative path to binary directory
    config['MacBuildDir']    = MacBuildDir      # relative path to build directory
    config['MacBuildDirQAT'] = MacBuildDirQAT   # relative path to build directory for QATest
    config['MacBuildLog']    = MacBuildLog      # relative path to build log file

    # (J) Extra parameters needed for deployment
    parameters['project_dir'] = ProjectDir

    # (K) Extra parameters needed for <pymod>
    #     <pymod> will be built if:
    #       BuildPymod   = True
    #       Platform     = [ 'Monterey', 'BigSur', 'Catalina' ]
    #       ModuleRuby   = [ 'Ruby31MacPorts', 'Ruby31Brew', 'RubyAnaconda3' ]
    #       ModulePython = [ 'Python38MacPorts', 'Python38Brew', 'Python39Brew',
    #                        'PythonAnaconda3',  'PythonAutoBrew' ]
    parameters['BuildPymod']   = BuildPymod
    parameters['Platform']     = Platform
    parameters['ModuleRuby']   = ModuleRuby
    parameters['ModulePython'] = ModulePython

    PymodDistDir = dict()
    if Platform in [ 'Ventura', 'Monterey', 'BigSur', 'Catalina' ]:
        if ModuleRuby in [ 'Ruby31MacPorts', 'Ruby31Brew', 'RubyAnaconda3', 'Ruby32MacPorts', 'Ruby32Brew' ]:
            if ModulePython in [ 'Python38MacPorts', 'Python39MacPorts' ]:
                PymodDistDir[ModulePython] = 'dist-MP3'
            elif ModulePython in [ 'Python38Brew', 'Python39Brew', 'PythonAutoBrew' ]:
                PymodDistDir[ModulePython] = 'dist-HB3'
            elif ModulePython in [ 'PythonAnaconda3' ]:
                PymodDistDir[ModulePython] = 'dist-ana3'
    parameters['pymod_dist'] = PymodDistDir
    return parameters

#------------------------------------------------------------------------------
## To run the "setup.py" script with appropriate options for building
#  the klayout Python Module "pymod".
#
# @param[in] parameters     dictionary containing the build parameters
#
# @return 0 on success; non-zero (1), otherwise
#------------------------------------------------------------------------------
def Build_pymod(parameters):
    #---------------------------------------------------------------------------
    # [1] <pymod> will be built if:
    #       BuildPymod   = True
    #       Platform     = [ 'Ventura', 'Monterey', 'BigSur', 'Catalina' ]
    #       ModuleRuby   = [ 'Ruby31MacPorts', 'Ruby31Brew', 'RubyAnaconda3',
    #                        'Ruby32MacPorts', 'Ruby32Brew' ]
    #       ModulePython = [ 'Python38MacPorts', 'Python38Brew', 'PythonAnaconda3',
    #                        'Python39MacPorts', 'Python39Brew', 'PythonAutoBrew' ]
    #---------------------------------------------------------------------------
    BuildPymod   = parameters['BuildPymod']
    Platform     = parameters['Platform']
    ModuleRuby   = parameters['ModuleRuby']
    ModulePython = parameters['ModulePython']
    if not BuildPymod:
        return 0
    if not Platform in [ 'Ventura', 'Monterey', 'BigSur', 'Catalina' ]:
        return 0
    elif not ModuleRuby in [ 'Ruby31MacPorts', 'Ruby31Brew', 'RubyAnaconda3', 'Ruby32MacPorts', 'Ruby32Brew' ]:
        return 0
    elif not ModulePython in [ 'Python38MacPorts', 'Python38Brew', 'PythonAnaconda3', \
                               'Python39MacPorts', 'Python39Brew', 'PythonAutoBrew' ]:
        return 0

    #--------------------------------------------------------------------
    # [2] Get the new directory names (dictionary) for "dist" and
    #     set the CPATH environment variable for including <png.h>
    #     required to build the pymod of 0.28 or later
    #--------------------------------------------------------------------
    PymodDistDir = parameters['pymod_dist']
    # Using MacPorts
    if PymodDistDir[ModulePython] == 'dist-MP3':
        addBinPath = "/opt/local/bin"
        addIncPath = "/opt/local/include"
        addLibPath = "/opt/local/lib"
    # Using Homebrew
    elif PymodDistDir[ModulePython] == 'dist-HB3':
        addBinPath = "%s/bin"     % DefaultHomebrewRoot  # defined in "build4mac_env.py"
        addIncPath = "%s/include" % DefaultHomebrewRoot  # -- ditto --
        addLibPath = "%s/lib"     % DefaultHomebrewRoot  # -- ditto --
    elif  PymodDistDir[ModulePython] == 'dist-ana3':
        addBinPath = "/Applications/anaconda3/bin"
        addIncPath = "/Applications/anaconda3/include"
        addLibPath = "/Applications/anaconda3/lib"
    else:
        addBinPath = ""
        addIncPath = ""
        addLibPath = ""

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
            os.environ['LDFLAGS'] = '-L%s' % addLibPath
        else:
            os.environ['LDFLAGS'] = '-L%s %s' % (addLibPath, ldpath)

    #--------------------------------------------------------------------
    # [3] Set different command line parameters for building <pymod>
    #--------------------------------------------------------------------
    cmd1_args = "   -m setup  build \\\n"
    cmd2_args = "   -m setup  bdist_wheel \\\n"
    deloc_cmd = "   delocate-wheel --ignore-missing-dependencies"
    cmd3_args = "   <wheel file> \\\n"
    cmd4_args = "   -m setup  clean --all \\\n"

    #--------------------------------------------------------------------
    # [4] Make the consolidated command lines
    #--------------------------------------------------------------------
    command1  = "time"
    command1 += " \\\n   %s \\\n" % parameters['python']
    command1 += cmd1_args
    command1 += "   2>&1 | tee -a %s; \\\n" % parameters['logfile']
    command1 += "   test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

    command2  = "time"
    command2 += " \\\n   %s \\\n" % parameters['python']
    command2 += cmd2_args
    command2 += "   2>&1 | tee -a %s; \\\n" % parameters['logfile']
    command2 += "   test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

    command3  = "time"
    command3 += " \\\n   %s \\\n" % deloc_cmd
    command3 += cmd3_args
    command3 += "   2>&1 | tee -a %s; \\\n" % parameters['logfile']
    command3 += "   test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

    command4  = "time"
    command4 += " \\\n   %s \\\n" % parameters['python']
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
    # [5] Invoke the main Python scripts; takes time:-)
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
    # Copy and relink library dependencies for wheel.
    #     In this step, the "delocate-wheel" command using the desired Python must be found in the PATH.
    #     Refer to: https://github.com/Kazzz-S/klayout/issues/49#issuecomment-1432154118
    #               https://pypi.org/project/delocate/
    #---------------------------------------------------------------------------------------------------------
    cmd3_args = glob.glob( "dist/*.whl" )  # like ['dist/klayout-0.28.6-cp39-cp39-macosx_12_0_x86_64.whl']
    if len(cmd3_args) == 1:
        command3  = "time"
        command3 += " \\\n   %s \\\n" % deloc_cmd
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

    #-----------------------------------------------------
    # [6] Rename the "dist/" directory
    #-----------------------------------------------------
    os.rename( "dist", PymodDistDir[ModulePython] )
    return 0

#------------------------------------------------------------------------------
## To run the main Bash script "build.sh" with appropriate options
#
# @param[in] parameters     dictionary containing the build parameters
#
# @return 0 on success; non-zero (1), otherwise
#------------------------------------------------------------------------------
def Run_Build_Command(parameters):
    jump2pymod = False  # default=False; set True to jump into pymod-build for debugging

    if not jump2pymod:
        #-----------------------------------------------------
        # [1] Set parameters passed to the main Bash script
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

        # (D) Qt5 | Qt6 (Homebrew)
        #cmd_args += " \\\n  -qt5" # make 'build.sh' detect the Qt type automatically
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

        # (G) options to `make` tool
        if 'make_options' in parameters:
            cmd_args += " \\\n  -option %s" % parameters['make_options']

        # (H) about Ruby
        if 'ruby' in parameters:
            cmd_args += " \\\n  -ruby   %s" % parameters['ruby']
            cmd_args += " \\\n  -rbinc  %s" % parameters['rbinc']
            cmd_args += " \\\n  -rblib  %s" % parameters['rblib']
            if 'rbinc2' in parameters:
                cmd_args += " \\\n  -rbinc2  %s" % parameters['rbinc2']
        else:
            cmd_args += " \\\n  -noruby"

        # (I) about Python
        if 'python' in parameters:
            cmd_args += " \\\n  -python %s" % parameters['python']
            cmd_args += " \\\n  -pyinc  %s" % parameters['pyinc']
            cmd_args += " \\\n  -pylib  %s" % parameters['pylib']
        else:
            cmd_args += " \\\n  -nopython"

        #-----------------------------------------------------
        # [2] Make the consolidated command line
        #-----------------------------------------------------
        command  = "time"
        command += " \\\n  %s" % parameters['build_cmd']
        command += cmd_args
        command += "  2>&1 | tee %s; \\\n" % parameters['logfile']
        command += "test ${PIPESTATUS[0]} -eq 0"  # tee always exits with 0

        if parameters['check_cmd_only']:
            print(command)
            sys.exit(0)

        #-----------------------------------------------------
        # [3] Invoke the main Bash script; takes time:-)
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
        # [4] Prepare "*.macQAT/" directory for the QATest.
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
        shutil.copy2( "macbuild/macQAT.sh", MacBuildDirQAT )
        shutil.copy2( "macbuild/macQAT.py", MacBuildDirQAT )
        print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
        print( "### <%s>: prepared the initial *.macQAT/" % myscript, file=sys.stderr )
        print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
        print( "", file=sys.stderr )

    #------------------------------------------------------------------------
    # [5] Build <pymod> for some predetermined environments on demand
    #------------------------------------------------------------------------
    BuildPymod = parameters['BuildPymod']
    if BuildPymod:
        ret = Build_pymod(parameters)
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

    BuildPymod     = parameters['BuildPymod']
    ProjectDir     = parameters['project_dir']
    MacBinDir      = parameters['bin']
    MacBuildDir    = parameters['build']
    MacBuildLog    = parameters['logfile']

    AbsMacPkgDir   = "%s/%s" % (ProjectDir, MacPkgDir)
    AbsMacBinDir   = "%s/%s" % (ProjectDir, MacBinDir)
    AbsMacBuildDir = "%s/%s" % (ProjectDir, MacBuildDir)
    AbsMacBuildLog = "%s/%s" % (ProjectDir, MacBuildLog)

    if BuildPymod:
        try:
            PymodDistDir = parameters['pymod_dist']
            pymodDistDir = PymodDistDir[ModulePython]   # [ 'dist-MP3', 'dist-HB3', 'dist-ana3' ]
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
    # [3] Create the directory skeleton for "klayout.app" bundle
    #     and command line buddy tools such as "strm2cif".
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
    #                             |              +-- 'db_plugins' --slink--> ../MacOS/db_plugins/
    #                             +-- MacOS/+
    #                             |         +-- 'klayout'
    #                             |         +-- db_plugins/
    #                             |         +-- lay_plugins/
    #                             +-- Buddy/+
    #                             |         +-- 'strm2cif'
    #                             |         +-- 'strm2dxf'
    #                             |         :
    #                             |         +-- 'strmxor'
    #                             |
    #                             +-- pymod-dist/+ (created only if *.whl and *.egg are available)
    #                                            +-- klayout-0.27.8-cp38-cp38-macosx_10_9_x86_64.whl (example)(1)
    #                                            +-- klayout-0.27.8-py3.8-macosx-10.9-x86_64.egg (example)(2)
    #
    #                                            (1) *.whl is recommended to install with 'pip3'
    #                                            (2) *.egg is for 'easy_install' users
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
    if BuildPymod and not pymodDistDir == "":
        os.makedirs(targetDirP)


    print( " [4] Copying KLayout's dynamic link libraries to 'Frameworks' ..." )
    #---------------------------------------------------------------------------------------
    # [4] Copy KLayout's dynamic link libraries to "Frameworks/" and create
    #     the library dependency dictionary.
    #     <<< Do this job in "Frameworks/" >>>
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
    #---------------------------------------------------------------------------------------
    os.chdir( targetDirF )
    dynamicLinkLibs = glob.glob( os.path.join( AbsMacBinDir, "*.dylib" ) )
    depDicOrdinary  = {} # inter-library dependency dictionary
    pathDic = {} # paths to insert for each library
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
            depDicOrdinary.update(dependDic)
            #-------------------------------------------------------------------
            # (C) This library goes into Frameworks, hence record it's path there
            #-------------------------------------------------------------------
            pathDic[nameStyle3] = "@executable_path/../Frameworks/" + nameStyle3

    os.chdir(ProjectDir)
    #-------------------------------------------------------------------
    # Copy the contents of the plugin directories to a place next to
    # the application binary
    #-------------------------------------------------------------------
    for piDir in [ "db_plugins", "lay_plugins" ]:
        os.makedirs( os.path.join( targetDirM, piDir ))
        dynamicLinkLibs = glob.glob( os.path.join( MacBinDir, piDir, "*.dylib" ) )
        for item in dynamicLinkLibs:
            if os.path.isfile(item) and not os.path.islink(item):
                #-------------------------------------------------------------------
                # (A) Copy an ordinary *.dylib file here by changing the name
                #     to style (3) and set its mode to 0755 (sanity check).
                #-------------------------------------------------------------------
                fullName = os.path.basename(item).split('.')
                # e.g. [ 'libklayout_lay', '0', '26', '1', 'dylib' ]
                nameStyle3 = fullName[0] + "." + fullName[1] + ".dylib"
                destPath = os.path.join( targetDirM, piDir, nameStyle3 )
                shutil.copy2( item, destPath )
                os.chmod( destPath, 0o0755 )
                #-------------------------------------------------------------------
                # (B) Then get inter-library dependencies
                #     Note that will pull all dependencies and sort them out later
                #     dropping those which don't have a path entry
                #-------------------------------------------------------------------
                otoolCm   = "otool -L %s | grep 'dylib'" % destPath
                otoolOut  = os.popen( otoolCm ).read()
                dependDic = DecomposeLibraryDependency(otoolOut)
                depDicOrdinary.update(dependDic)
                #-------------------------------------------------------------------
                # (C) This library goes into the plugin dir
                #-------------------------------------------------------------------
                pathDic[nameStyle3] = "@executable_path/" + piDir + "/" + nameStyle3

    '''
    PrintLibraryDependencyDictionary( depDicOrdinary, pathDic, "Style (3)" )
    exit()
    '''

    #----------------------------------------------------------------------------------
    # (D) Make a symbolic link
    #        'db_plugins' --slink--> ../MacOS/db_plugins/
    #     under Frameworks/, which is required for the command line Buddy tools.
    #
    #     Ref. https://github.com/KLayout/klayout/issues/460#issuecomment-571803458
    #
    #             :
    #             +-- Frameworks/+
    #             |              +-- '*.framework'
    #             |              +-- '*.dylib'
    #             |              +-- 'db_plugins' --slink--> ../MacOS/db_plugins/
    #             +-- MacOS/+
    #             |         +-- 'klayout'
    #             |         +-- db_plugins/
    #             |         +-- lay_plugins/
    #             :
    #----------------------------------------------------------------------------------
    os.chdir( targetDirF )
    os.symlink( "../MacOS/db_plugins/", "./db_plugins" )


    print( " [5] Setting and changing the identification names among KLayout's libraries ..." )
    #-------------------------------------------------------------
    # [5] Set the identification names for KLayout's libraries
    #     and make the library aware of the locations of libraries
    #     on which it depends; that is, inter-library dependency
    #-------------------------------------------------------------
    ret = SetChangeIdentificationNameOfDyLib( depDicOrdinary, pathDic )
    if not ret == 0:
        msg = "!!! Failed to set and change to new identification names !!!"
        print(msg)
        return 1


    print( " [6] Copying built executables and resource files ..." )
    #-------------------------------------------------------------
    # [6] Copy some known files in source directories to
    #     relevant target directories
    #-------------------------------------------------------------
    os.chdir(ProjectDir)
    sourceDir0 = "%s/klayout.app/Contents" % MacBinDir
    sourceDir1 = sourceDir0 + "/MacOS"
    sourceDir2 = "%s/macbuild/Resources" % ProjectDir
    sourceDir3 = "%s" % MacBinDir
    sourceDir4 = "%s/pymod" % MacBinDir

    # (A) the main components
    tmpfileM = ProjectDir + "/macbuild/Resources/Info.plist.template"
    keydicM  = { 'exe': 'klayout', 'icon': 'klayout.icns', 'bname': 'klayout', 'ver': Version }
    plistM   = GenerateInfoPlist( keydicM, tmpfileM )
    file     = open( targetDir0 + "/Info.plist", "w" )
    file.write(plistM)
    file.close()

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
    if BuildPymod and not pymodDistDir == "":
        for item in glob.glob( pymodDistDir + "/*.whl" ):
            shutil.copy2( item,  targetDirP )
        for item in glob.glob( pymodDistDir + "/*.egg" ):
            shutil.copy2( item,  targetDirP )

    print( " [7] Setting and changing the identification names of KLayout's libraries in each executable ..." )
    #-------------------------------------------------------------
    # [7] Set and change the library identification name(s) of
    #     different executables
    #-------------------------------------------------------------
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
        print( " [8] Finally, deploying Qt's Frameworks ..." )
        #-------------------------------------------------------------
        # [8] Deploy Qt Frameworks
        #-------------------------------------------------------------
        verbose = " -verbose=%d" % DeployVerbose
        app_bundle = "klayout.app"
        options = macdepQtOpt + verbose
        deploytool = parameters['deploy_tool']

        # Without the following, the plugin cocoa would not be found properly.
        shutil.copy2( sourceDir2 + "/qt.conf", targetDirM )
        os.chmod( targetDirM + "/qt.conf",      0o0644 )

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
        # [9] Special deployment of Python3.8 or newer from Homebrew
        #     To use Python3.8 from Homebrew on Catalina...
        #       in "/usr/local/opt/python@3.8/lib/"
        #             Python.framework -> ../Frameworks/Python.framework/ <=== this symbolic was needed
        #             pkgconfig/
        #
        #     Use the "python3HB.py" tool to make different symbolic links [*] including the above one.
        #        Catalina0{kazzz-s} lib (1)% pwd
        #        /usr/local/opt/python@3.8/lib
        #        Catalina0{kazzz-s} lib (2)% ll
        #        total 0
        #        drwxr-xr-x  4 kazzz-s admin 128 12 16 21:40 .
        #        drwxr-xr-x 13 kazzz-s admin 416 12 12 23:08 ..
        #    [*] lrwxr-xr-x  1 kazzz-s admin  31 12 16 21:40 Python.framework -> ../Frameworks/Python.framework/
        #        drwxr-xr-x  4 kazzz-s admin 128 12 12 23:08 pkgconfig
        #
        #        Catalina0{kazzz-s} Python.framework (3)% pwd
        #        /usr/local/opt/python@3.8/Frameworks/Python.framework/Versions
        #        Catalina0{kazzz-s} Versions (4)% ll
        #        total 0
        #        drwxr-xr-x 4 kazzz-s admin 128 12 16 21:40 .
        #        drwxr-xr-x 6 kazzz-s admin 192 12 16 21:40 ..
        #        drwxr-xr-x 9 kazzz-s admin 288 12 12 23:08 3.8
        #    [*] lrwxr-xr-x 1 kazzz-s admin   4 12 16 21:40 Current -> 3.8/
        #
        #        Catalina0{kazzz-s} Python.framework (5)% pwd
        #        /usr/local/opt/python@3.8/Frameworks/Python.framework
        #        Catalina0{kazzz-s} Python.framework (6)% ll
        #        total 0
        #        drwxr-xr-x 6 kazzz-s admin 192 12 16 21:40 .
        #        drwxr-xr-x 3 kazzz-s admin  96 12 12 23:07 ..
        #    [*] lrwxr-xr-x 1 kazzz-s admin  25 12 16 21:40 Headers -> Versions/Current/Headers/
        #    [*] lrwxr-xr-x 1 kazzz-s admin  23 12 16 21:40 Python -> Versions/Current/Python
        #    [*] lrwxr-xr-x 1 kazzz-s admin  27 12 16 21:40 Resources -> Versions/Current/Resources/
        #        drwxr-xr-x 4 kazzz-s admin 128 12 16 21:40 Versions
        #-----------------------------------------------------------------------------------------------
        deploymentPython38HB   = (ModulePython == 'Python38Brew')
        deploymentPython39HB   = (ModulePython == 'Python39Brew')
        deploymentPythonAutoHB = (ModulePython == 'PythonAutoBrew')
        if (deploymentPython38HB or deploymentPython39HB or deploymentPythonAutoHB) and NonOSStdLang:
            from build4mac_util import WalkFrameworkPaths, PerformChanges

            if deploymentPython38HB:
                HBPythonFrameworkPath = HBPython38FrameworkPath
                pythonHBVer           = "3.8" # 'pinned' to this version as of KLayout version 0.26.7 (2020-09-13)
            elif deploymentPython39HB:
                HBPythonFrameworkPath = HBPython39FrameworkPath
                pythonHBVer           = "3.9" # 'pinned' to this version as of KLayout version 0.28.2 (2023-01-02)
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
                    msg = "command failed: %s"
                    print( msg % command, file=sys.stderr )
                    sys.exit(1)

            shutil.copy2( sourceDir2 + "/start-console.py", targetDirM )
            shutil.copy2( sourceDir2 + "/klayout_console",  targetDirM )
            os.chmod( targetDirM + "/start-console.py", 0o0755 )
            os.chmod( targetDirM + "/klayout_console",  0o0755 )

            print( "  [9.2] Relinking dylib dependencies inside Python.framework" )
            print( "   [9.2.1] Patching Python Framework" )
            depdict = WalkFrameworkPaths( pythonFrameworkPath )
            appPythonFrameworkPath = '@executable_path/../Frameworks/Python.framework/'
            PerformChanges( depdict, [(HBPythonFrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs )

            print( "   [9.2.2] Patching 'Python' itself in Python Framework" )
            filterreg = r'\t+%s/(opt|Cellar)' % DefaultHomebrewRoot
            Patch_Python_In_PythonFramework( pythonFrameworkPath, filter_regex=filterreg  )

            print( "   [9.2.3] Patching %s/opt/ libs" % DefaultHomebrewRoot ) # eg. DefaultHomebrewRoot == "/usr/local"
            usrLocalPath    = '%s/opt/' % DefaultHomebrewRoot
            appUsrLocalPath = '@executable_path/../Frameworks/'
            replacePairs    = [ (usrLocalPath, appUsrLocalPath, True) ]
            filterreg       = r'\t+%s/(opt|Cellar)' % DefaultHomebrewRoot
            depdict         = WalkFrameworkPaths( pythonFrameworkPath, search_path_filter=filterreg )
            PerformChanges( depdict, replacePairs, bundleExecPathAbs )

            print( "   [9.2.4] Patching openssl@1.1, gdbm, readline, sqlite, xz" )
            usrLocalPath    = '%s/opt/' % DefaultHomebrewRoot
            appUsrLocalPath = '@executable_path/../Frameworks/'
            replacePairs    = [ (usrLocalPath, appUsrLocalPath, True) ]
            replacePairs.extend( [ (openssl_version, '@executable_path/../Frameworks/openssl@1.1', True)
                for openssl_version in glob.glob( '%s/Cellar/openssl@1.1/*' % DefaultHomebrewRoot ) ] )
            filterreg = r'\t+%s/(opt|Cellar)' % DefaultHomebrewRoot
            depdict   = WalkFrameworkPaths( [pythonFrameworkPath + '/../openssl@1.1',
                                             pythonFrameworkPath + '/../gdbm',
                                             pythonFrameworkPath + '/../readline',
                                             pythonFrameworkPath + '/../sqlite',
                                             pythonFrameworkPath + '/../xz'], search_path_filter=filterreg )

            PerformChanges( depdict, replacePairs, bundleExecPathAbs )

            print( "  [9.3] Relinking dylib dependencies for klayout" )
            klayoutPath = bundleExecPathAbs
            depdict     = WalkFrameworkPaths( klayoutPath, filter_regex=r'klayout$' )
            PerformChanges( depdict, [(HBPythonFrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs )

            libKlayoutPath = bundleExecPathAbs + '../Frameworks'
            depdict        = WalkFrameworkPaths( libKlayoutPath, filter_regex=r'libklayout' )
            PerformChanges( depdict, [(HBPythonFrameworkPath, appPythonFrameworkPath, False)], bundleExecPathAbs )

            print( "  [9.4] Patching site.py, pip/, and distutils/" )
            site_module = "%s/Versions/%s/lib/python%s/site.py" % (pythonFrameworkPath, pythonHBVer, pythonHBVer)
            with open(site_module, 'r') as site:
                buf = site.readlines()
            with open(site_module, 'w') as site:
                import re
                for line in buf:
                    # This will fool pip into thinking it's inside a virtual environment
                    # and install new packages to the correct site-packages
                    if re.match("^PREFIXES", line) is not None:
                        line = line + "sys.real_prefix = sys.prefix\n"
                    # do not allow installation in the user folder.
                    if re.match("^ENABLE_USER_SITE", line) is not None:
                        line = "ENABLE_USER_SITE = False\n"
                    site.write(line)

            #----------------------------------------------------------------------------------
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
            # >>> pip.main( ['install', 'numpy'] )
            # >>> pip.main( ['install', 'scipy'] )
            # >>> pip.main( ['install', 'pandas'] )
            # >>> pip.main( ['install', 'matplotlib'] )
            #----------------------------------------------------------------------------------
            pip_module = "%s/Versions/%s/lib/python%s/site-packages/pip/__init__.py" % \
                                     (pythonFrameworkPath, pythonHBVer, pythonHBVer)
            with open(pip_module, 'r') as pip:
                buf = pip.readlines()
            with open(pip_module, 'w') as pip:
                import re
                for line in buf:
                    # this will reject user's configuration of pip, forcing the isolated mode
                    line = re.sub("return isolated$", "return isolated or True", line)
                    pip.write(line)

            distutilsconfig = "%s/Versions/%s/lib/python%s/distutils/distutils.cfg" % \
                                                (pythonFrameworkPath, pythonHBVer, pythonHBVer)
            with open(distutilsconfig, 'r') as file:
                buf = file.readlines()
            with open(distutilsconfig, 'w') as file:
                import re
                for line in buf:
                    # This will cause all packages to be installed to sys.prefix
                    if re.match('prefix=', line) is not None:
                        continue
                    file.write(line)

        #-------------------------------------------------------------
        # [10] Special deployment of Ruby3.1 from Homebrew?
        #-------------------------------------------------------------
        deploymentRuby31HB = (ModuleRuby == 'Ruby31Brew')
        if deploymentRuby31HB and NonOSStdLang:

            print( "" )
            print( " [10] You have reached optional deployment of Ruby from %s ..." % HBRuby31Path )
            print( "   [!!!] Sorry, the deployed package will not work properly since deployment of" )
            print( "         Ruby2.7 from Homebrew is not yet supported." )
            print( "         Since you have Homebrew development environment, there two options:" )
            print( "           (1) Retry to make a package with '-Y|--DEPLOY' option." )
            print( "               This will not deploy any of Qt[5|6], Python, and Ruby from Homebrew." )
            print( "               Instead, the package will directly use those Frameworks and libraries" )
            print( "               in your Homebrew environment." )
            print( "           (2) Rebuild KLayout with '-r|--ruby <nil|Sys>' option depending on your preference." )
            print( "" )

    else:
        print( " [8] Skipped deploying Qt's Frameworks and optional Python/Ruby Frameworks..." )
    print( "##### Finished deploying libraries and executables for <klayout.app> #####" )
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
        ret = Run_Build_Command(parameters)
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
