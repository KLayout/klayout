#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac_env.py"
#
# Here are dictionaries of ...
#  different modules for building KLayout (http://www.klayout.de/index.php)
#  version 0.28.17 or later on different Apple Mac OSX platforms.
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
                'deploy': '/opt/local/libexec/qt5/bin/macdeployqt'
              }

# Qt5 from Homebrew (https://brew.sh/)
#   install with 'brew install qt5'
# [Key Type Name] = 'Qt5Brew'
Qt5Brew = { 'qmake' : '%s/opt/qt@5/bin/qmake' % DefaultHomebrewRoot,
            'deploy': '%s/opt/qt@5/bin/macdeployqt' % DefaultHomebrewRoot
          }

# Qt5 bundled with anaconda3 installed under /Applications/anaconda3/
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Qt5Ana3'
Qt5Ana3 = { 'qmake' : '/Applications/anaconda3/bin/qmake',
            'deploy': '/Applications/anaconda3/bin/macdeployqt'
          }

#-------------------------------------------------------------------------
# Whereabouts of different components of Qt6    *+*+*+ EXPERIMENTAL *+*+*+
#-------------------------------------------------------------------------
# Qt6 from MacPorts (https://www.macports.org/)
#   install with 'sudo port install [qt6|qt6-qttools]'
# [Key Type Name] = 'Qt6MacPorts'
Qt6MacPorts = { 'qmake' : '/opt/local/libexec/qt6/bin/qmake',
                'deploy': '/opt/local/libexec/qt6/bin/macdeployqt'
              }

# Qt6 from Homebrew (https://brew.sh/)
#   install with 'brew install qt6'
# [Key Type Name] = 'Qt6Brew'
Qt6Brew = { 'qmake' : '%s/opt/qt@6/bin/qmake' % DefaultHomebrewRoot,
            'deploy': '%s/opt/qt@6/bin/macdeployqt' % DefaultHomebrewRoot
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
RubySys  = [ 'RubyMonterey', 'RubyVentura', 'RubySonoma' ]
RubyExt  = [ 'Ruby33MacPorts', 'Ruby33Brew', 'RubyAnaconda3' ]
Rubies   = RubyNil + RubySys + RubyExt

#-----------------------------------------------------
# Whereabouts of different components of Ruby
#-----------------------------------------------------
# Bundled with Monterey (12.x)
# [Key Type Name] = 'Sys'
MontereySDK     = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
RubyMonterey    = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % MontereySDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % MontereySDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % MontereySDK
                  }

# Bundled with Ventura (13.x)
# [Key Type Name] = 'Sys'
VenturaSDK      = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
RubyVentura     = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % VenturaSDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % VenturaSDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % VenturaSDK
                  }

# Bundled with Sonoma (14.x)
# [Key Type Name] = 'Sys'
SonomaSDK       = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
RubySonoma      = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % SonomaSDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % SonomaSDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % SonomaSDK
                  }

# Ruby 3.3 from MacPorts (https://www.macports.org/)
#  install with 'sudo port install ruby33'
# [Key Type Name] = 'MP33'
Ruby33MacPorts  = { 'exe': '/opt/local/bin/ruby3.3',
                    'inc': '/opt/local/include/ruby-3.3.0',
                    'lib': '/opt/local/lib/libruby.3.3.dylib'
                  }

# Ruby 3.3 from Homebrew
#   install with 'brew install ruby@3.3'
# [Key Type Name] = 'HB33'
HBRuby33Path    = '%s/opt/ruby@3.3' % DefaultHomebrewRoot
Ruby33Brew      = { 'exe': '%s/bin/ruby' % HBRuby33Path,
                    'inc': '%s/include/ruby-3.3.0' % HBRuby33Path,
                    'lib': '%s/lib/libruby.3.3.dylib' % HBRuby33Path
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
                    'Ruby33MacPorts': Ruby33MacPorts,
                    'Ruby33Brew'    : Ruby33Brew,
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
PythonSys  = [ 'PythonMonterey', 'PythonVentura', 'PythonSonoma' ]
PythonExt  = [ 'Python39MacPorts', 'Python39Brew' ]
PythonExt += [ 'Python311MacPorts', 'Python311Brew' ]
PythonExt += [ 'PythonAnaconda3', 'PythonAutoBrew' ]
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

# Python 3.9 from MacPorts (https://www.macports.org/)
#   install with 'sudo port install python39'
# [Key Type Name] = 'MP39'
Python39MacPorts  = { 'exe': '/opt/local/Library/Frameworks/Python.framework/Versions/3.9/bin/python3.9',
                      'inc': '/opt/local/Library/Frameworks/Python.framework/Versions/3.9/include/python3.9',
                      'lib': '/opt/local/Library/Frameworks/Python.framework/Versions/3.9/lib/libpython3.9.dylib'
                    }

# Python 3.11 from MacPorts (https://www.macports.org/)
#   install with 'sudo port install python311'
# [Key Type Name] = 'MP311'
Python311MacPorts = { 'exe': '/opt/local/Library/Frameworks/Python.framework/Versions/3.11/bin/python3.11',
                      'inc': '/opt/local/Library/Frameworks/Python.framework/Versions/3.11/include/python3.11',
                      'lib': '/opt/local/Library/Frameworks/Python.framework/Versions/3.11/lib/libpython3.11.dylib'
                    }

# Python 3.9 from Homebrew
#   install with 'brew install python@3.9'
# [Key Type Name] = 'HB39'
HBPython39FrameworkPath = '%s/opt/python@3.9/Frameworks/Python.framework' % DefaultHomebrewRoot
Python39Brew      = { 'exe': '%s/Versions/3.9/bin/python3.9' % HBPython39FrameworkPath,
                      'inc': '%s/Versions/3.9/include/python3.9' % HBPython39FrameworkPath,
                      'lib': '%s/Versions/3.9/lib/libpython3.9.dylib' % HBPython39FrameworkPath
                    }

# Python 3.11 from Homebrew
#   install with 'brew install python@3.11'
# [Key Type Name] = 'HB311'
HBPython311FrameworkPath = '%s/opt/python@3.11/Frameworks/Python.framework' % DefaultHomebrewRoot
Python311Brew     = { 'exe': '%s/Versions/3.11/bin/python3.11' % HBPython311FrameworkPath,
                      'inc': '%s/Versions/3.11/include/python3.11' % HBPython311FrameworkPath,
                      'lib': '%s/Versions/3.11/lib/libpython3.11.dylib' % HBPython311FrameworkPath
                    }

# Python 3.11 bundled with anaconda3 installed under /Applications/anaconda3/
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Ana3'
PythonAnaconda3 = { 'exe': '/Applications/anaconda3/bin/python3.11',
                    'inc': '/Applications/anaconda3/include/python3.11',
                    'lib': '/Applications/anaconda3/lib/libpython3.11.dylib'
                  }

# Latest Python from Homebrew
#   install with 'brew install python'
#   There can be multiple candidates such as: (python, python3, python@3, python@3.8, python@3.9,
#                                              python@3.10, python@3.11, python@3.12, python@3.13 )
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
    # when I have [python3, python@3, python@3.8, python@3.9, python@3.10, python@3.11]
    print(HBPythonAutoFrameworkPath)
    print(HBAutoFrameworkVersionPath)
    print(HBPythonAutoVersion)
    print(PythonAutoBrew)
    quit()

    /usr/local/opt/python@3.11/Frameworks/Python.framework
    /usr/local/opt/python@3.11/Frameworks/Python.framework/Versions
    3.11
    { 'exe': '/usr/local/opt/python@3.11/Frameworks/Python.framework/Versions/3.11/bin/python3.11',
      'inc': '/usr/local/opt/python@3.11/Frameworks/Python.framework/Versions/3.11/include/python3.11',
      'lib': '/usr/local/opt/python@3.11/Frameworks/Python.framework/Versions/3.11/lib/libpython3.11.dylib'
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
                     'Python39MacPorts' : Python39MacPorts,
                     'Python311MacPorts': Python311MacPorts,
                     'Python39Brew'     : Python39Brew,
                     'Python311Brew'    : Python311Brew,
                     'PythonAnaconda3'  : PythonAnaconda3
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
