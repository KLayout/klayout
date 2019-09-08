#! /usr/bin/env python
# -*- coding: utf-8 -*-

#===============================================================================
# File: "macbuild/build4mac_env.py"
#
# Here are dictionaries of ...
#  different modules for building KLayout (http://www.klayout.de/index.php)
#  version 0.25 or later on different Apple Mac OSX platforms.
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
# [Key Type Name] = 'Qt5MacPorts'
Qt5MacPorts = { 'qmake' : '/opt/local/libexec/qt5/bin/qmake',
                'deploy': '/opt/local/libexec/qt5/bin/macdeployqt'
              }

# Qt5 from Homebrew (https://brew.sh/)
# install with 'brew install qt'
# [Key Type Name] = 'Qt5Brew'
Qt5Brew = { 'qmake' : '/usr/local/opt/qt/bin/qmake',
            'deploy': '/usr/local/opt/qt/bin/macdeployqt'
          }

# Qt5 bundled with anaconda3 installed under $HOME/anaconda3/
# [Key Type Name] = 'Qt5Ana3'
Qt5Ana3 = { 'qmake' : '%s/anaconda3/bin/qmake' % MyHome,
            'deploy': '%s/anaconda3/bin/macdeployqt' % MyHome
          }

# Qt5 Custom (https://www1.qt.io/offline-installers/)

#-----------------------------------------------------
# [2] Ruby
#-----------------------------------------------------
Rubies  = [ 'nil', 'RubyYosemite', 'RubyElCapitan', 'RubySierra', 'RubyHighSierra' ]
Rubies += [ 'RubyMojave', 'Ruby24MacPorts', 'Ruby25Brew' ]
Rubies += [ 'RubyAnaconda3' ]

#-----------------------------------------------------
# Whereabout of different components of Ruby
#-----------------------------------------------------
# Bundled with Yosemite (10.10)
# [Key Type Name] = 'Sys'
RubyYosemite    = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/lib/libruby.dylib'
                  }

# Bundled with El Capitan (10.11)
# [Key Type Name] = 'Sys'
RubyElCapitan   = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/lib/libruby.dylib'
                  }

# Bundled with Sierra (10.12)
# [Key Type Name] = 'Sys'
RubySierra      = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/lib/libruby.dylib'
                  }

# Bundled with High Sierra (10.13)
# [Key Type Name] = 'Sys'
RubyHighSierra  = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/lib/libruby.dylib'
                  }

# Bundled with Mojave (10.14)
# [Key Type Name] = 'Sys'
RubyMojave      = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/lib/libruby.dylib'
                  }

# Ruby 2.4 from MacPorts (https://www.macports.org/) *+*+*+ EXPERIMENTAL *+*+*+
# [Key Type Name] = 'MP24'
Ruby24MacPorts  = { 'exe': '/opt/local/bin/ruby2.4',
                    'inc': '/opt/local/include/ruby-2.4.0',
                    'lib': '/opt/local/lib/libruby.2.4.dylib'
                  }

# Ruby 2.5 from Brew *+*+*+ EXPERIMENTAL *+*+*+
# [Key Type Name] = 'Brew25'
Ruby25Brew  = { 'exe': '/usr/local/bin/ruby',
                'inc': '/usr/local/include/ruby-2.5.0',
                'lib': '/usr/local/lib/libruby.2.5.0.dylib'
              }

# Ruby 2.5 bundled with anaconda3 installed under $HOME/anaconda3/ *+*+*+ EXPERIMENTAL *+*+*+
# If the version is 2.4.x in your anaconda3, change the version strings accordingly.
# [Key Type Name] = 'RubyAnaconda3'
RubyAnaconda3 = { 'exe': '%s/anaconda3/bin/ruby' % MyHome,
                  'inc': '%s/anaconda3/include/ruby-2.5.0' % MyHome,
                  'lib': '%s/anaconda3/lib/libruby.2.5.1.dylib' % MyHome
                }

# Consolidated dictionary kit for Ruby
RubyDictionary  = { 'nil'           : None,
                    'RubyYosemite'  : RubyYosemite,
                    'RubyElCapitan' : RubyElCapitan,
                    'RubySierra'    : RubySierra,
                    'RubyHighSierra': RubyHighSierra,
                    'RubyMojave'    : RubyMojave,
                    'Ruby24MacPorts': Ruby24MacPorts,
                    'Ruby25Brew'    : Ruby25Brew,
                    'RubyAnaconda3' : RubyAnaconda3
                  }

#-----------------------------------------------------
# [3] Python
#-----------------------------------------------------
Pythons  = [ 'nil', 'PythonYosemite', 'PythonElCapitan', 'PythonSierra', 'PythonHighSierra' ]
Pythons += [ 'PythonMojave', 'Python36MacPorts', 'Python37Brew' ]
Pythons += [ 'PythonAnaconda3' ]

#-----------------------------------------------------
# Whereabout of different components of Python
#-----------------------------------------------------
# Bundled with Yosemite (10.10)
# [Key Type Name] = 'Sys'
PythonYosemite  = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with El Capitan (10.11)
# [Key Type Name] = 'Sys'
PythonElCapitan = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with Sierra (10.12)
# [Key Type Name] = 'Sys'
PythonSierra    = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with High Sierra (10.13)
# [Key Type Name] = 'Sys'
PythonHighSierra= { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with Mojave (10.14)
# [Key Type Name] = 'Sys'
PythonMojave    = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Python 3.6 from MacPorts (https://www.macports.org/) *+*+*+ EXPERIMENTAL *+*+*+
# [Key Type Name] = 'MP36'
Python36MacPorts= { 'exe': '/opt/local/Library/Frameworks/Python.framework/Versions/3.6/bin/python3.6m' ,
                    'inc': '/opt/local/Library/Frameworks/Python.framework/Versions/3.6/include/python3.6m',
                    'lib': '/opt/local/Library/Frameworks/Python.framework/Versions/3.6/lib/libpython3.6m.dylib'
                  }

# Python 3.7 from Brew *+*+*+ EXPERIMENTAL *+*+*+
# [Key Type Name] = 'pybrew'
Python37Brew    = { 'exe': '/usr/local/opt/python/libexec/bin/python' ,
                    'inc': '/usr/local/opt/python/Frameworks/Python.framework/Versions/3.7/Headers',
                    'lib': '/usr/local/opt/python/Frameworks/Python.framework/Versions/3.7/Python'
                  }

# Python 3.7 bundled with anaconda3 installed under $HOME/anaconda3/ *+*+*+ EXPERIMENTAL *+*+*+
# [Key Type Name] = 'PythonAnaconda3'
PythonAnaconda3 = { 'exe': '%s/anaconda3/bin/python' % MyHome,
                    'inc': '%s/anaconda3/include/python3.7m' % MyHome,
                    'lib': '%s/anaconda3/lib/libpython3.7m.dylib' % MyHome
                  }

# Consolidated dictionary kit for Python
PythonDictionary= { 'nil'             : None,
                    'PythonYosemite'  : PythonYosemite,
                    'PythonElCapitan' : PythonElCapitan,
                    'PythonSierra'    : PythonSierra,
                    'PythonHighSierra': PythonHighSierra,
                    'PythonMojave'    : PythonMojave,
                    'Python36MacPorts': Python36MacPorts,
                    'Python37Brew'    : Python37Brew,
                    'PythonAnaconda3' : PythonAnaconda3
                  }

#-----------------------------------------------------
# [4] KLayout executables including buddy tools
#-----------------------------------------------------
KLayoutExecs  = ['klayout']
KLayoutExecs += ['strm2cif', 'strm2dxf', 'strm2gds', 'strm2gdstxt', 'strm2oas']
KLayoutExecs += ['strm2txt', 'strmclip', 'strmcmp',  'strmrun',     'strmxor']

#----------------
# End of File
#----------------
