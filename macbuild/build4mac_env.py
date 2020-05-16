#! /usr/bin/env python
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
MyHome = os.environ['HOME']

#-----------------------------------------------------
# [0] Xcode's tools
#-----------------------------------------------------
XcodeToolChain = { 'nameID': '/usr/bin/install_name_tool -id ',
                   'nameCH': '/usr/bin/install_name_tool -change '
                 }

#-----------------------------------------------------
# [1] Qt
#-----------------------------------------------------
Qts = [ 'Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3' ]

#-----------------------------------------------------
# Whereabout of different components of Qt5
#-----------------------------------------------------
# Qt5 from MacPorts (https://www.macports.org/)
#   install with 'sudo port install qt5'
# [Key Type Name] = 'Qt5MacPorts'
Qt5MacPorts = { 'qmake' : '/opt/local/libexec/qt5/bin/qmake',
                'deploy': '/opt/local/libexec/qt5/bin/macdeployqt'
              }

# Qt5 from Homebrew (https://brew.sh/)
#   install with 'brew install qt'
# [Key Type Name] = 'Qt5Brew'
Qt5Brew = { 'qmake' : '/usr/local/opt/qt/bin/qmake',
            'deploy': '/usr/local/opt/qt/bin/macdeployqt'
          }

# Qt5 bundled with anaconda3 installed under /Applications/anaconda3/
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Qt5Ana3'
Qt5Ana3 = { 'qmake' : '/Applications/anaconda3/bin/qmake',
            'deploy': '/Applications/anaconda3/bin/macdeployqt'
          }

#-----------------------------------------------------
# [2] Ruby
#-----------------------------------------------------
RubyNil = [ 'nil' ]
RubySys = [ 'RubyElCapitan', 'RubySierra', 'RubyHighSierra', 'RubyMojave', 'RubyCatalina' ]
RubyExt = [ 'Ruby26MacPorts', 'Ruby27Brew', 'RubyAnaconda3' ]
Rubies  = RubyNil + RubySys + RubyExt

#-----------------------------------------------------
# Whereabout of different components of Ruby
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
# [Key Type Name] = 'Sys'
CatalinaSDK = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.15.sdk"
RubyCatalina    = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/bin/ruby',
                    'inc': '%s/System/Library/Frameworks/Ruby.framework/Headers' % CatalinaSDK,
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.6/usr/lib/libruby.dylib'
                  }

# Ruby 2.6 from MacPorts (https://www.macports.org/) *+*+*+ EXPERIMENTAL *+*+*+
#  install with 'sudo port install ruby26'
# [Key Type Name] = 'MP26'
Ruby26MacPorts  = { 'exe': '/opt/local/bin/ruby2.6',
                    'inc': '/opt/local/include/ruby-2.6.0',
                    'lib': '/opt/local/lib/libruby.2.6.dylib'
                  }

# Ruby 2.7 from Homebrew *+*+*+ EXPERIMENTAL *+*+*+
#   install with 'brew install ruby'
# [Key Type Name] = 'HB27'
HBRuby27Path    = '/usr/local/opt/ruby'
Ruby27Brew      = { 'exe': '%s/bin/ruby' % HBRuby27Path,
                    'inc': '%s/include/ruby-2.7.0' % HBRuby27Path,
                    'lib': '%s/lib/libruby.2.7.dylib' % HBRuby27Path
                  }

# Ruby 2.5 bundled with anaconda3 installed under /Applications/anaconda3/ *+*+*+ EXPERIMENTAL *+*+*+
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Ana3'
RubyAnaconda3   = { 'exe': '/Applications/anaconda3/bin/ruby',
                    'inc': '/Applications/anaconda3/include/ruby-2.5.0',
                    'lib': '/Applications/anaconda3/lib/libruby.2.5.1.dylib'
                  }

# Consolidated dictionary kit for Ruby
RubyDictionary  = { 'nil'           : None,
                    'RubyYosemite'  : RubyYosemite,
                    'RubyElCapitan' : RubyElCapitan,
                    'RubySierra'    : RubySierra,
                    'RubyHighSierra': RubyHighSierra,
                    'RubyMojave'    : RubyMojave,
                    'RubyCatalina'  : RubyCatalina,
                    'Ruby26MacPorts': Ruby26MacPorts,
                    'Ruby27Brew'    : Ruby27Brew,
                    'RubyAnaconda3' : RubyAnaconda3
                  }

#-----------------------------------------------------
# [3] Python
#-----------------------------------------------------
PythonNil = [ 'nil' ]
PythonSys = [ 'PythonElCapitan', 'PythonSierra', 'PythonHighSierra', 'PythonMojave', 'PythonCatalina' ]
PythonExt = [ 'Python37MacPorts', 'Python37Brew', 'PythonAnaconda3' ]
Pythons   = PythonNil + PythonSys + PythonExt

#-----------------------------------------------------
# Whereabout of different components of Python
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

# Bundled with Mojave (10.15)
# [Key Type Name] = 'Sys'
PythonCatalina  = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python',
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Python 3.7 from MacPorts (https://www.macports.org/) *+*+*+ EXPERIMENTAL *+*+*+
#   install with 'sudo port install python37'
# [Key Type Name] = 'MP37'
Python37MacPorts= { 'exe': '/opt/local/Library/Frameworks/Python.framework/Versions/3.7/bin/python3.7m',
                    'inc': '/opt/local/Library/Frameworks/Python.framework/Versions/3.7/include/python3.7m',
                    'lib': '/opt/local/Library/Frameworks/Python.framework/Versions/3.7/lib/libpython3.7m.dylib'
                  }

# Python 3.7 from Homebrew *+*+*+ EXPERIMENTAL *+*+*+
#   install with 'brew install python'
# [Key Type Name] = 'HB37'
HBPython37FrameworkPath = '/usr/local/opt/python3/Frameworks/Python.framework'
Python37Brew    = { 'exe': '%s/Versions/3.7/bin/python3.7m' % HBPython37FrameworkPath,
                    'inc': '%s/Versions/3.7/include/python3.7m' % HBPython37FrameworkPath,
                    'lib': '%s/Versions/3.7/lib/libpython3.7m.dylib' % HBPython37FrameworkPath
                  }

# Python 3.7 bundled with anaconda3 installed under /Applications/anaconda3/ *+*+*+ EXPERIMENTAL *+*+*+
# The standard installation deploys the tool under $HOME/opt/anaconda3/.
# If so, you need to make a symbolic link: /Applications/anaconda3 ---> $HOME/opt/anaconda3/
# [Key Type Name] = 'Ana3'
PythonAnaconda3 = { 'exe': '/Applications/anaconda3/bin/python3.7m',
                    'inc': '/Applications/anaconda3/include/python3.7m',
                    'lib': '/Applications/anaconda3/lib/libpython3.7m.dylib'
                  }

# Consolidated dictionary kit for Python
PythonDictionary= { 'nil'             : None,
                    'PythonElCapitan' : PythonElCapitan,
                    'PythonSierra'    : PythonSierra,
                    'PythonHighSierra': PythonHighSierra,
                    'PythonMojave'    : PythonMojave,
                    'PythonCatalina'  : PythonCatalina,
                    'Python37MacPorts': Python37MacPorts,
                    'Python37Brew'    : Python37Brew,
                    'PythonAnaconda3' : PythonAnaconda3
                  }

#-----------------------------------------------------
# [4] KLayout executables including buddy tools
#-----------------------------------------------------
KLayoutExecs  = [ 'klayout' ]
KLayoutExecs += [ 'strm2cif', 'strm2dxf', 'strm2gds', 'strm2gdstxt', 'strm2oas' ]
KLayoutExecs += [ 'strm2txt', 'strmclip', 'strmcmp',  'strmrun',     'strmxor'  ]

#----------------
# End of File
#----------------
