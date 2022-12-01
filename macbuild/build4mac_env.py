#! /usr/bin/env python3
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac_env.py"
#
# Here are dictionaries of ...
#  different modules for building KLayout (http://www.klayout.de/index.php)
#  version 0.26.1 or later on different Apple Mac OSX platforms.
#
# This file is imported by 'build4mac.py' script.
#===============================================================================
import os
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
else:
  DefaultHomebrewRoot = '/usr/local'
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
#-----------------------------------------------------
RubyNil  = [ 'nil' ]
RubySys  = [ 'RubyElCapitan', 'RubySierra', 'RubyHighSierra', 'RubyMojave' ]
RubySys += [ 'RubyCatalina', 'RubyBigSur', 'RubyMonterey' ]
RubyExt  = [ 'Ruby31MacPorts', 'Ruby31Brew', 'RubyAnaconda3' ]
Rubies   = RubyNil + RubySys + RubyExt

#-----------------------------------------------------
# Whereabouts of different components of Ruby
#-----------------------------------------------------
# Bundled with Yosemite (10.10)
#   !!! Yosemite is no longer supported (KLayout 0.26 ~) but remains here to keep the record of
#       the directory structure of earlier generations.
# [Key Type Name] = 'Sys'
RubyYosemite    = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/bin/ruby',
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/lib/libruby.dylib'
                  }

# Bundled with El Capitan (10.11)
# [Key Type Name] = 'Sys'
RubyElCapitan   = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/bin/ruby',
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/lib/libruby.dylib'
                  }

# Bundled with Sierra (10.12)
# [Key Type Name] = 'Sys'
RubySierra      = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/bin/ruby',
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/lib/libruby.dylib'
                  }

# Bundled with High Sierra (10.13)
# [Key Type Name] = 'Sys'
RubyHighSierra  = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/bin/ruby',
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/lib/libruby.dylib'
                  }

# Bundled with Mojave (10.14)
#   The missing Ruby header files under "/System/Library/Frameworks/Ruby.framework/" were manually deployed there
#   from "Xcode-10.1-beta2" with the corresponding Ruby version.
# [Key Type Name] = 'Sys'
RubyMojave      = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/bin/ruby',
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/lib/libruby.dylib'
                  }

# Bundled with Catalina (10.15)
#   !!! Catalina does not allow to hack the "/System" directory; it's READ ONLY even for the super user!
#       Hence, we need to refer to the Ruby header file in "Xcode.app" directly.
#
#   With the major release of "macOS Big Sur (11.0)" in November 2020, Xcode has been updated, too.
#     (base) MacBookPro2{kazzz-s}(1)$ pwd
#     /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/include/ruby-2.6.0
#
#     (base) MacBookPro2{kazzz-s}(2)$ ll
#     total 4
#     drwxr-xr-x  6 root wheel 192 11 15 20:57 .
#     drwxr-xr-x  3 root wheel  96 10 20 05:33 ..
#     drwxr-xr-x 23 root wheel 736 10 24 11:57 ruby
#     -rw-r--r--  1 root wheel 868 10 19 19:32 ruby.h
#     lrwxr-xr-x  1 root wheel  19 11 15 20:57 universal-darwin19 -> universal-darwin20/ <=== manually created this symbolic link
#     drwxr-xr-x  6 root wheel 192 10 20 05:33 universal-darwin20
# [Key Type Name] = 'Sys'
CatalinaSDK     = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
RubyCatalina    = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % CatalinaSDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % CatalinaSDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % CatalinaSDK
                  }

# Bundled with Big Sur (11.x)
# Refer to the "Catalina" section above
# [Key Type Name] = 'Sys'
BigSurSDK       = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
RubyBigSur      = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % BigSurSDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % BigSurSDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % BigSurSDK
                  }

# Bundled with Monterey (12.x)
# Refer to the "Catalina" section above
# [Key Type Name] = 'Sys'
MontereySDK     = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
RubyMonterey    = { 'exe':  '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc':  '%s/System/Library/Frameworks/Ruby.framework/Headers' % MontereySDK,
                    'inc2': '%s/System/Library/Frameworks/Ruby.framework/Headers/ruby' % MontereySDK,
                    'lib':  '%s/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.tbd' % MontereySDK
                  }

# Ruby 3.1 from MacPorts (https://www.macports.org/) *+*+*+ EXPERIMENTAL *+*+*+
#  install with 'sudo port install ruby31'
# [Key Type Name] = 'MP31'
Ruby31MacPorts  = { 'exe': '/opt/local/bin/ruby3.1',
                    'inc': '/opt/local/include/ruby-3.1.3',
                    'lib': '/opt/local/lib/libruby.3.1.dylib'
                  }

# Ruby 3.1 from Homebrew *+*+*+ EXPERIMENTAL *+*+*+
#   install with 'brew install ruby@3.1'
# [Key Type Name] = 'HB31'
HBRuby31Path    = '%s/opt/ruby@3.1' % DefaultHomebrewRoot
Ruby31Brew      = { 'exe': '%s/bin/ruby' % HBRuby31Path,
                    'inc': '%s/include/ruby-3.1.0' % HBRuby31Path,
                    'lib': '%s/lib/libruby.3.1.dylib' % HBRuby31Path
                  }

# Ruby 3.1 bundled with anaconda3 installed under /Applications/anaconda3/ *+*+*+ EXPERIMENTAL *+*+*+
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Ana3'
RubyAnaconda3   = { 'exe': '/Applications/anaconda3/bin/ruby',
                    'inc': '/Applications/anaconda3/include/ruby-3.1.0',
                    'lib': '/Applications/anaconda3/lib/libruby.3.1.dylib'
                  }

# Consolidated dictionary kit for Ruby
RubyDictionary  = { 'nil'           : None,
                    'RubyYosemite'  : RubyYosemite,
                    'RubyElCapitan' : RubyElCapitan,
                    'RubySierra'    : RubySierra,
                    'RubyHighSierra': RubyHighSierra,
                    'RubyMojave'    : RubyMojave,
                    'RubyCatalina'  : RubyCatalina,
                    'RubyBigSur'    : RubyBigSur,
                    'RubyMonterey'  : RubyMonterey,
                    'Ruby31MacPorts': Ruby31MacPorts,
                    'Ruby31Brew'    : Ruby31Brew,
                    'RubyAnaconda3' : RubyAnaconda3
                  }

#-----------------------------------------------------
# [3] Python
#-----------------------------------------------------
PythonNil  = [ 'nil' ]
PythonSys  = [ 'PythonElCapitan', 'PythonSierra', 'PythonHighSierra', 'PythonMojave' ]
PythonSys += [ 'PythonCatalina', 'PythonBigSur', 'PythonMonterey' ]
PythonExt  = [ 'Python38MacPorts', 'Python38Brew', 'Python39Brew', 'PythonAnaconda3', 'PythonAutoBrew' ]
Pythons    = PythonNil + PythonSys + PythonExt

#-----------------------------------------------------
# Whereabouts of different components of Python
#-----------------------------------------------------
# Bundled with Yosemite (10.10)
#   !!! Yosemite is no longer supported but remains here to keep the record of the directory structure
#       of earlier generations.
# [Key Type Name] = 'Sys'
PythonYosemite  = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with El Capitan (10.11)
# [Key Type Name] = 'Sys'
PythonElCapitan = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with Sierra (10.12)
# [Key Type Name] = 'Sys'
PythonSierra    = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with High Sierra (10.13)
# [Key Type Name] = 'Sys'
PythonHighSierra= { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with Mojave (10.14)
# [Key Type Name] = 'Sys'
PythonMojave    = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with Catalina (10.15)
# [Key Type Name] = 'Sys'
PythonCatalina  = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with Big Sur (11.x)
# ** IMPORTANT NOTES **
#    Xcode [13.1 .. ] does not allow to link the legacy Python 2.7 library not to produce unsupported applications.
#    This code block remains here to keep the parallelism.
# [Key Type Name] = 'Sys'
PythonBigSur    = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with Monterey (12.x)
# ** IMPORTANT NOTES **
#    Xcode [13.1 .. ] does not allow to link the legacy Python 2.7 library not to produce unsupported applications.
#    This code block remains here to keep the parallelism.
# [Key Type Name] = 'Sys'
PythonMonterey  = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Python 3.8 from MacPorts (https://www.macports.org/) *+*+*+ EXPERIMENTAL *+*+*+
#   install with 'sudo port install python38'
# [Key Type Name] = 'MP38'
Python38MacPorts= { 'exe': '/opt/local/Library/Frameworks/Python.framework/Versions/3.8/bin/python3.8',
                    'inc': '/opt/local/Library/Frameworks/Python.framework/Versions/3.8/include/python3.8',
                    'lib': '/opt/local/Library/Frameworks/Python.framework/Versions/3.8/lib/libpython3.8.dylib'
                  }

# Python 3.8 from Homebrew *+*+*+ EXPERIMENTAL *+*+*+
#   install with 'brew install python@3.8'
# [Key Type Name] = 'HB38'
HBPython38FrameworkPath = '%s/opt/python@3.8/Frameworks/Python.framework' % DefaultHomebrewRoot
Python38Brew    = { 'exe': '%s/Versions/3.8/bin/python3.8' % HBPython38FrameworkPath,
                    'inc': '%s/Versions/3.8/include/python3.8' % HBPython38FrameworkPath,
                    'lib': '%s/Versions/3.8/lib/libpython3.8.dylib' % HBPython38FrameworkPath
                  }

# Python 3.9 from Homebrew *+*+*+ EXPERIMENTAL *+*+*+
#   install with 'brew install python@3.9'
# [Key Type Name] = 'HB39'
HBPython39FrameworkPath = '%s/opt/python@3.9/Frameworks/Python.framework' % DefaultHomebrewRoot
Python39Brew    = { 'exe': '%s/Versions/3.9/bin/python3.9' % HBPython39FrameworkPath,
                    'inc': '%s/Versions/3.9/include/python3.9' % HBPython39FrameworkPath,
                    'lib': '%s/Versions/3.9/lib/libpython3.9.dylib' % HBPython39FrameworkPath
                  }

# Python 3.8 bundled with anaconda3 installed under /Applications/anaconda3/ *+*+*+ EXPERIMENTAL *+*+*+
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Ana3'
PythonAnaconda3 = { 'exe': '/Applications/anaconda3/bin/python3.8',
                    'inc': '/Applications/anaconda3/include/python3.8',
                    'lib': '/Applications/anaconda3/lib/libpython3.8.dylib'
                  }

# Latest Python from Homebrew *+*+*+ EXPERIMENTAL *+*+*+
#   install with 'brew install python'
#   There can be multiple candidates such as: (python, python3, python@3, python@3.8, python@3.9)
#   Hard to tell which is going to be available to the user. Picking the last one.
# [Key Type Name] = 'HBAuto'
HBPythonAutoFrameworkPath = ""
HBPythonAutoVersion       = ""
try:
    HBPythonAutoFrameworkPath = glob.glob( "%s/opt/python*/Frameworks/Python.framework" % DefaultHomebrewRoot )[-1]
    # expand 3* into HBPythonAutoVersion, there should be only one, but I am taking no chances.
    HBAutoFrameworkVersionPath, HBPythonAutoVersion = os.path.split( glob.glob( "%s/Versions/3*" % HBPythonAutoFrameworkPath )[0] )
    PythonAutoBrew  = { 'exe': '%s/%s/bin/python%s' % ( HBAutoFrameworkVersionPath, HBPythonAutoVersion, HBPythonAutoVersion ),
                        'inc': '%s/%s/include/python%s' % ( HBAutoFrameworkVersionPath, HBPythonAutoVersion, HBPythonAutoVersion ),
                        'lib': glob.glob( "%s/%s/lib/*.dylib" % ( HBAutoFrameworkVersionPath, HBPythonAutoVersion ) )[0]
                      }
except Exception as e:
    _have_Homebrew_Python = False
    print( "  WARNING!!! Since you don't have the Homebrew Python Frameworks, you cannot use the '-p HBAuto' option. " )
    pass
else:
    _have_Homebrew_Python = True

# Consolidated dictionary kit for Python
PythonDictionary = { 'nil'             : None,
                     'PythonElCapitan' : PythonElCapitan,
                     'PythonSierra'    : PythonSierra,
                     'PythonHighSierra': PythonHighSierra,
                     'PythonMojave'    : PythonMojave,
                     'PythonCatalina'  : PythonCatalina,
                     'PythonBigSur'    : PythonBigSur,
                     'PythonMonterey'  : PythonMonterey,
                     'Python38MacPorts': Python38MacPorts,
                     'Python38Brew'    : Python38Brew,
                     'Python39Brew'    : Python39Brew,
                     'PythonAnaconda3' : PythonAnaconda3
                   }
if _have_Homebrew_Python:
    PythonDictionary['PythonAutoBrew'] = PythonAutoBrew

#-----------------------------------------------------
# [4] KLayout executables including buddy tools
#-----------------------------------------------------
KLayoutExecs  = [ 'klayout' ]
KLayoutExecs += [ 'strm2cif', 'strm2dxf', 'strm2gds', 'strm2gdstxt', 'strm2oas' ]
KLayoutExecs += [ 'strm2txt', 'strmclip', 'strmcmp',  'strmrun',     'strmxor'  ]

#----------------
# End of File
#----------------
