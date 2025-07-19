#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac_env.py"
#
# Here are dictionaries of ...
#  different modules for building KLayout (http://www.klayout.de/index.php)
#  version 0.30.2 or later on different Apple Mac OSX platforms.
#
# This file is imported by 'build4mac.py' script.
#===============================================================================
import os
import re
import glob
import platform

#---------------------------------------------------------------------------------------------------
# [0] Xcode's tools
#       and
#     Default Homebrew root
#       Ref. https://github.com/Homebrew/brew/blob/master/docs/Installation.md#alternative-installs
#---------------------------------------------------------------------------------------------------
XcodeToolChain = { 'nameID': '/usr/bin/install_name_tool -id ',
                   'nameCH': '/usr/bin/install_name_tool -change '
                 }

(System, Node, Release, MacVersion, Machine, Processor) = platform.uname()
if Machine == "arm64": # Apple Silicon!
  DefaultHomebrewRoot = '/opt/homebrew'
  HomebrewSearchPathFilter1 = '\t+%s/opt' % DefaultHomebrewRoot
  HomebrewSearchPathFilter2 = '\t+@loader_path/../../../../../../../../../../opt'
  HomebrewSearchPathFilter3 =    '@loader_path/../../../../../../../../../../opt' # no leading white space
  # 1: absolute path as seen in ~python@3.9.17
  # 2: relative path as seen in  python@3.9.18
else:
  DefaultHomebrewRoot = '/usr/local'
  HomebrewSearchPathFilter1 = '\t+%s/opt' % DefaultHomebrewRoot
  HomebrewSearchPathFilter2 = '\t+@loader_path/../../../../../../../../../../opt'
  HomebrewSearchPathFilter3 =    '@loader_path/../../../../../../../../../../opt' # no leading white space
  # 1: absolute path as seen in ~python@3.9.17
  #      BigSur{kazzz-s} lib-dynload (1)% otool -L _sqlite3.cpython-39-darwin.so
  #      _sqlite3.cpython-39-darwin.so:
  #   ===> /usr/local/opt/sqlite/lib/libsqlite3.0.dylib (compatibility version 9.0.0, current version 9.6.0)
  #        /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1292.100.5)
  #
  # 2: relative path as seen in python@3.9.18
  #      Monterey{kazzz-s} lib-dynload (1)% otool -L _sqlite3.cpython-39-darwin.so
  #      _sqlite3.cpython-39-darwin.so:
  #   ===> @loader_path/../../../../../../../../../../opt/sqlite/lib/libsqlite3.0.dylib (compatibility version 9.0.0, current version 9.6.0)
  #        /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1311.100.3)
  #
  # 3. absolute path again as seen in python@3.11
  #      Monterey{kazzz-s} lib-dynload (1)% otool -L _sqlite3.cpython-311-darwin.so
  #      _sqlite3.cpython-311-darwin.so:
  #   ===> /usr/local/opt/sqlite/lib/libsqlite3.0.dylib (compatibility version 9.0.0, current version 9.6.0)
  #        /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1311.100.3)
  #
  # Ref. https://github.com/Homebrew/homebrew-core/issues/140930#issuecomment-1701524467
del System, Node, Release, MacVersion, Machine, Processor

#-----------------------------------------------------
# [1] Qt5 or Qt6
#-----------------------------------------------------
Qts  = [ 'Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3' ]
Qts += [ 'Qt6MacPorts', 'Qt6Brew' ]

#-----------------------------------------------------
# Whereabouts of different components of Qt5
#-----------------------------------------------------
# Qt5 from MacPorts (https://www.macports.org/)
#   install with 'sudo port install [qt5|qt5-qttools]'
# [Key Type Name] = 'Qt5MacPorts'
Qt5MacPorts = { 'qmake' : '/opt/local/libexec/qt5/bin/qmake',
                'deploy': '/opt/local/libexec/qt5/bin/macdeployqt',
                'libdir': '/opt/local/libexec/qt5/lib'
              }

# Qt5 from Homebrew (https://brew.sh/)
#   install with 'brew install qt5'
# [Key Type Name] = 'Qt5Brew'
Qt5Brew = { 'qmake' : '%s/opt/qt@5/bin/qmake' % DefaultHomebrewRoot,
            'deploy': '%s/opt/qt@5/bin/macdeployqt' % DefaultHomebrewRoot,
            'libdir': '%s/opt/qt@5/lib' % DefaultHomebrewRoot
          }

# Qt5 bundled with anaconda3 installed under /Applications/anaconda3/
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Qt5Ana3'
Qt5Ana3 = { 'qmake' : '/Applications/anaconda3/bin/qmake',
            'deploy': '/Applications/anaconda3/bin/macdeployqt',
            'libdir': '/Applications/anaconda3/lib'
          }

#-------------------------------------------------------------------------
# Whereabouts of different components of Qt6    *+*+*+ EXPERIMENTAL *+*+*+
#-------------------------------------------------------------------------
# Qt6 from MacPorts (https://www.macports.org/)
#   install with 'sudo port install [qt6|qt6-qttools]'
# [Key Type Name] = 'Qt6MacPorts'
Qt6MacPorts = { 'qmake' : '/opt/local/libexec/qt6/bin/qmake',
                'deploy': '/opt/local/libexec/qt6/bin/macdeployqt',
                'libdir': '/opt/local/libexec/qt6/lib'
              }

# Qt6 from Homebrew (https://brew.sh/)
#   install with 'brew install qt6'
# [Key Type Name] = 'Qt6Brew'
Qt6Brew = { 'qmake' : '%s/opt/qt@6/bin/qmake' % DefaultHomebrewRoot,
            'deploy': '%s/opt/qt@6/bin/macdeployqt' % DefaultHomebrewRoot,
            'libdir': '%s/opt/qt@6/lib' % DefaultHomebrewRoot
          }

# Consolidated dictionary kit for Qt[5|6]
Qt56Dictionary  = { 'Qt5MacPorts': Qt5MacPorts,
                    'Qt5Brew'    : Qt5Brew,
                    'Qt5Ana3'    : Qt5Ana3,
                    'Qt6MacPorts': Qt6MacPorts,
                    'Qt6Brew'    : Qt6Brew
                  }

#-----------------------------------------------------
# [2] Ruby
#     * Dropped the followings (2023-10-24).
#         Sys: [ElCapitan - BigSur]
#         Ext: [Ruby31]
#     * See 415b5aa2efca04928f1148a69e77efd5d76f8c1d
#           for the previous states.
#-----------------------------------------------------
RubyNil  = [ 'nil' ]
RubySys  = [ 'RubyMonterey', 'RubyVentura', 'RubySonoma', 'RubySequoia' ]
RubyExt  = [ 'Ruby33MacPorts', 'Ruby34Brew', 'RubyAnaconda3' ]
Rubies   = RubyNil + RubySys + RubyExt

#-----------------------------------------------------
# Whereabouts of different components of Ruby
#-----------------------------------------------------
# % which ruby
#   /System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby
#
# % ruby -v
#   ruby 2.6.10p210 (2022-04-12 revision 67958) [universal.x86_64-darwin21]
#
# Where is the 'ruby.h' used to build the 'ruby' executable?
#
# % ruby -e "puts File.expand_path('ruby.h', RbConfig::CONFIG['rubyhdrdir'])"
#   ===> /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.1.sdk \
#        /System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/include/ruby-2.6.0/ruby.h
#
# Bundled with Monterey (12.x)
# [Key Type Name] = 'Sys'
MontereyXcSDK   = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
MontereyCLTSDK  = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
RubyMonterey    = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % MontereyXcSDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % MontereyXcSDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % MontereyXcSDK
                  }

# Bundled with Ventura (13.x)
# [Key Type Name] = 'Sys'
VenturaXcSDK    = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
VenturaCLTSDK   = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
RubyVentura     = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % VenturaXcSDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % VenturaXcSDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % VenturaXcSDK
                  }

# Bundled with Sonoma (14.x)
# [Key Type Name] = 'Sys'
SonomaXcSDK     = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
SonomaCLTSDK    = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
RubySonoma      = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % SonomaXcSDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % SonomaXcSDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % SonomaXcSDK
                  }

# Bundled with Sequoia (15.x)
# [Key Type Name] = 'Sys'
SequoiaXcSDK     = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
SequoiaCLTSDK    = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk"
RubySequoia      = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                     'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % SequoiaXcSDK,
                     'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % SequoiaXcSDK,
                     'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % SequoiaXcSDK
                   }

# Ruby 3.3 from MacPorts (https://www.macports.org/)
#  install with 'sudo port install ruby33'
# [Key Type Name] = 'MP33'
Ruby33MacPorts  = { 'exe': '/opt/local/bin/ruby3.3',
                    'inc': '/opt/local/include/ruby-3.3.8',
                    'lib': '/opt/local/lib/libruby.3.3.dylib'
                  }

# Ruby 3.4 from Homebrew
#   install with 'brew install ruby@3.4'
# [Key Type Name] = 'HB34'
HBRuby34Path    = '%s/opt/ruby@3.4' % DefaultHomebrewRoot
Ruby34Brew      = { 'exe': '%s/bin/ruby' % HBRuby34Path,
                    'inc': '%s/include/ruby-3.4.0' % HBRuby34Path,
                    'lib': '%s/lib/libruby.3.4.dylib' % HBRuby34Path
                  }

# Ruby 3.2 bundled with anaconda3 installed under /Applications/anaconda3/
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Ana3'
RubyAnaconda3   = { 'exe': '/Applications/anaconda3/bin/ruby',
                    'inc': '/Applications/anaconda3/include/ruby-3.2.0',
                    'lib': '/Applications/anaconda3/lib/libruby.3.2.dylib'
                  }

# Consolidated dictionary kit for Ruby
RubyDictionary  = { 'nil'           : None,
                    'RubyMonterey'  : RubyMonterey,
                    'RubyVentura'   : RubyVentura,
                    'RubySonoma'    : RubySonoma,
                    'RubySequoia'   : RubySequoia,
                    'Ruby33MacPorts': Ruby33MacPorts,
                    'Ruby34Brew'    : Ruby34Brew,
                    'RubyAnaconda3' : RubyAnaconda3
                  }

#-----------------------------------------------------
# [3] Python
#     * Dropped the followings (2023-10-24).
#         Sys: [ElCapitan - Monterey]
#         Ext: [Python38]
#     * See 415b5aa2efca04928f1148a69e77efd5d76f8c1d
#           for the previous states.
#-----------------------------------------------------
PythonNil  = [ 'nil' ]
PythonSys  = [ 'PythonMonterey', 'PythonVentura', 'PythonSonoma', 'PythonSequoia' ]
PythonExt  = [ 'Python311MacPorts', 'Python312MacPorts' ]
PythonExt += [ 'Python311Brew', 'Python312Brew', 'PythonAutoBrew' ]
PythonExt += [ 'PythonAnaconda3' ]
Pythons    = PythonNil + PythonSys + PythonExt

#-----------------------------------------------------
# Whereabouts of different components of Python
#-----------------------------------------------------
# Bundled with Monterey (12.x)
# [Key Type Name] = 'Sys'
MontereyPy3FWXc = "/Applications/Xcode.app/Contents/Developer/Library/Frameworks"
MontereyPy3FW   = "/Library/Developer/CommandLineTools/Library/Frameworks"
PythonMonterey  = { 'exe': '%s/Python3.framework/Versions/3.9/bin/python3.9' % MontereyPy3FW,
                    'inc': '%s/Python3.framework/Versions/3.9/include/python3.9' % MontereyPy3FW,
                    'lib': '%s/Python3.framework/Versions/3.9/lib/libpython3.9.dylib' % MontereyPy3FW
                  }

# Bundled with Ventura (13.x)
# [Key Type Name] = 'Sys'
VenturaPy3FWXc  = "/Applications/Xcode.app/Contents/Developer/Library/Frameworks"
VenturaPy3FW    = "/Library/Developer/CommandLineTools/Library/Frameworks"
PythonVentura   = { 'exe': '%s/Python3.framework/Versions/3.9/bin/python3.9' % VenturaPy3FW,
                    'inc': '%s/Python3.framework/Versions/3.9/include/python3.9' % VenturaPy3FW,
                    'lib': '%s/Python3.framework/Versions/3.9/lib/libpython3.9.dylib' % VenturaPy3FW
                  }

# Bundled with Sonoma (14.x)
# [Key Type Name] = 'Sys'
SonomaPy3FWXc   = "/Applications/Xcode.app/Contents/Developer/Library/Frameworks"
SonomaPy3FW     = "/Library/Developer/CommandLineTools/Library/Frameworks"
PythonSonoma    = { 'exe': '%s/Python3.framework/Versions/3.9/bin/python3.9' % SonomaPy3FW,
                    'inc': '%s/Python3.framework/Versions/3.9/include/python3.9' % SonomaPy3FW,
                    'lib': '%s/Python3.framework/Versions/3.9/lib/libpython3.9.dylib' % SonomaPy3FW
                  }

# Bundled with Sequoia (15.x)
# [Key Type Name] = 'Sys'
SequoiaPy3FWXc   = "/Applications/Xcode.app/Contents/Developer/Library/Frameworks"
SequoiaPy3FW     = "/Library/Developer/CommandLineTools/Library/Frameworks"
PythonSequoia    = { 'exe': '%s/Python3.framework/Versions/3.9/bin/python3.9' % SequoiaPy3FW,
                     'inc': '%s/Python3.framework/Versions/3.9/include/python3.9' % SequoiaPy3FW,
                     'lib': '%s/Python3.framework/Versions/3.9/lib/libpython3.9.dylib' % SequoiaPy3FW
                   }

# Python 3.11 from MacPorts (https://www.macports.org/)
#   install with 'sudo port install python311'
# [Key Type Name] = 'MP311'
Python311MacPorts = { 'exe': '/opt/local/Library/Frameworks/Python.framework/Versions/3.11/bin/python3.11',
                      'inc': '/opt/local/Library/Frameworks/Python.framework/Versions/3.11/include/python3.11',
                      'lib': '/opt/local/Library/Frameworks/Python.framework/Versions/3.11/lib/libpython3.11.dylib'
                    }

# Python 3.12 from MacPorts (https://www.macports.org/)
#   install with 'sudo port install python312'
# [Key Type Name] = 'MP312'
Python312MacPorts = { 'exe': '/opt/local/Library/Frameworks/Python.framework/Versions/3.12/bin/python3.12',
                      'inc': '/opt/local/Library/Frameworks/Python.framework/Versions/3.12/include/python3.12',
                      'lib': '/opt/local/Library/Frameworks/Python.framework/Versions/3.12/lib/libpython3.12.dylib'
                    }

# Python 3.11 from Homebrew
#   install with 'brew install python@3.11'
# [Key Type Name] = 'HB311'
HBPython311FrameworkPath = '%s/opt/python@3.11/Frameworks/Python.framework' % DefaultHomebrewRoot
Python311Brew     = { 'exe': '%s/Versions/3.11/bin/python3.11' % HBPython311FrameworkPath,
                      'inc': '%s/Versions/3.11/include/python3.11' % HBPython311FrameworkPath,
                      'lib': '%s/Versions/3.11/lib/libpython3.11.dylib' % HBPython311FrameworkPath
                    }

# Python 3.12 from Homebrew
#   install with 'brew install python@3.12'
# [Key Type Name] = 'HB312'
HBPython312FrameworkPath = '%s/opt/python@3.12/Frameworks/Python.framework' % DefaultHomebrewRoot
Python312Brew     = { 'exe': '%s/Versions/3.12/bin/python3.12' % HBPython312FrameworkPath,
                      'inc': '%s/Versions/3.12/include/python3.12' % HBPython312FrameworkPath,
                      'lib': '%s/Versions/3.12/lib/libpython3.12.dylib' % HBPython312FrameworkPath
                    }

# Python 3.12 bundled with anaconda3 installed under /Applications/anaconda3/
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Ana3'
PythonAnaconda3 = { 'exe': '/Applications/anaconda3/bin/python3.12',
                    'inc': '/Applications/anaconda3/include/python3.12',
                    'lib': '/Applications/anaconda3/lib/libpython3.12.dylib'
                  }

# Latest Python from Homebrew
#   install with 'brew install python'
#   There can be multiple candidates such as: (python, python3, python@3, python@3.8, python@3.9,
#                                              python@3.10, python@3.12, python@3.12, python@3.13 )
#   Hard to tell which is going to be available to the user. Picking the last one.
# [Key Type Name] = 'HBAuto'
HBPythonAutoFrameworkPath = ""
HBPythonAutoVersion       = ""
try:
    patPF = r"(%s/opt/python)([@]?)(3[.]?)([0-9]*)(/Frameworks/Python[.]framework)" % DefaultHomebrewRoot
    regPF = re.compile(patPF)
    dicPy = dict()
    for item in glob.glob( "%s/opt/python*/Frameworks/Python.framework" % DefaultHomebrewRoot ):
        if regPF.match(item):
            pyver = regPF.match(item).groups()[3] # ([0-9]*)
            if pyver == "":
                pyver = "0"
            dicPy[ int(pyver) ] = ( item, "3."+pyver )
    keys = sorted( dicPy.keys(), reverse=True )
    HBPythonAutoFrameworkPath = dicPy[keys[0]][0]
    HBPythonAutoVersion       = dicPy[keys[0]][1]

    HBAutoFrameworkVersionPath, HBPythonAutoVersion = os.path.split( glob.glob( "%s/Versions/3*" % HBPythonAutoFrameworkPath )[0] )
    PythonAutoBrew  = { 'exe': '%s/%s/bin/python%s' % ( HBAutoFrameworkVersionPath, HBPythonAutoVersion, HBPythonAutoVersion ),
                        'inc': '%s/%s/include/python%s' % ( HBAutoFrameworkVersionPath, HBPythonAutoVersion, HBPythonAutoVersion ),
                        'lib': glob.glob( "%s/%s/lib/*.dylib" % ( HBAutoFrameworkVersionPath, HBPythonAutoVersion ) )[0]
                      }
    """
    # when I have [python3, python@3, python@3.11, python@3.12]
    print(HBPythonAutoFrameworkPath)
    print(HBAutoFrameworkVersionPath)
    print(HBPythonAutoVersion)
    print(PythonAutoBrew)
    quit()

    /usr/local/opt/python@3.12/Frameworks/Python.framework
    /usr/local/opt/python@3.12/Frameworks/Python.framework/Versions
    3.12
    { 'exe': '/usr/local/opt/python@3.12/Frameworks/Python.framework/Versions/3.12/bin/python3.12',
      'inc': '/usr/local/opt/python@3.12/Frameworks/Python.framework/Versions/3.12/include/python3.12',
      'lib': '/usr/local/opt/python@3.12/Frameworks/Python.framework/Versions/3.12/lib/libpython3.12.dylib'
    }
    """
except Exception as e:
    _have_Homebrew_Python = False
    print( "  WARNING!!! Since you don't have the Homebrew Python Frameworks, you cannot use the '-p HBAuto' option. " )
    pass
else:
    _have_Homebrew_Python = True

# Consolidated dictionary kit for Python
PythonDictionary = { 'nil'              : None,
                     'PythonMonterey'   : PythonMonterey,
                     'PythonVentura'    : PythonVentura,
                     'PythonSonoma'     : PythonSonoma,
                     'PythonSequoia'    : PythonSequoia,
                     'Python312MacPorts': Python312MacPorts,
                     'Python312Brew'    : Python312Brew,
                     'PythonAnaconda3'  : PythonAnaconda3,
                     'Python311MacPorts': Python311MacPorts,
                     'Python311Brew'    : Python311Brew
                   }
if _have_Homebrew_Python:
    PythonDictionary['PythonAutoBrew'] = PythonAutoBrew

#-----------------------------------------------------
# [4] KLayout executables including buddy tools
#-----------------------------------------------------
KLayoutExecs  = [ 'klayout' ]
KLayoutExecs += [ 'strm2cif', 'strm2dxf', 'strm2gds', 'strm2gdstxt', 'strm2mag', 'strm2oas' ]
KLayoutExecs += [ 'strm2txt', 'strmclip', 'strmcmp',  'strmrun',     'strmxor'  ]

#----------------
# End of File
#----------------
