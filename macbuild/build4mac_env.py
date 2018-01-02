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

#-----------------------------------------------------
# [0] Xcode's tools
#-----------------------------------------------------
XcodeToolChain = { 'nameID': '/usr/bin/install_name_tool -id ',
                   'nameCH': '/usr/bin/install_name_tool -change '
                 }

#-----------------------------------------------------
# [1] Qt
#-----------------------------------------------------
Qts = [ 'Qt4MacPorts', 'Qt5MacPorts' ]

#-----------------------------------------------------
# Whereabout of different components of Qt4
#-----------------------------------------------------
# Qt4 from MacPorts (https://www.macports.org/)
Qt4MacPorts = { 'qmake' : '/opt/local/libexec/qt4/bin/qmake',
                'deploy': '/opt/local/libexec/qt4/bin/macdeployqt'
              }

#-----------------------------------------------------
# Whereabout of different components of Qt5
#-----------------------------------------------------
# Qt5 from MacPorts (https://www.macports.org/)
Qt5MacPorts = { 'qmake' : '/opt/local/libexec/qt5/bin/qmake',
                'deploy': '/opt/local/libexec/qt5/bin/macdeployqt'
              }

#-----------------------------------------------------
# [2] Ruby
#-----------------------------------------------------
Rubies  = [ 'nil', 'RubyYosemite', 'RubyElCapitan', 'RubySierra', 'RubyHighSierra' ]
Rubies += [ 'Ruby24SrcBuild' ]

#-----------------------------------------------------
# Whereabout of different components of Ruby
#-----------------------------------------------------
# Bundled with Yosemite (10.10)
RubyYosemite    = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/lib/libruby.dylib'
                  }

# Bundled with El Capitan (10.11)
RubyElCapitan   = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/lib/libruby.dylib'
                  }

# Bundled with Sierra (10.12)
RubySierra      = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.0/usr/lib/libruby.dylib'
                  }

# Bundled with High Sierra (10.13)
RubyHighSierra  = { 'exe': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/bin/ruby' ,
                    'inc': '/System/Library/Frameworks/Ruby.framework/Headers',
                    'lib': '/System/Library/Frameworks/Ruby.framework/Versions/2.3/usr/lib/libruby.dylib'
                  }

# Ruby 2.4 built from source code (https://github.com/ruby): *+*+*+ EXPERIMENTAL *+*+*+
#   configured by:
#       $ ./configure --prefix=$HOME/Ruby24/ --enable-shared
Ruby24SrcBuild  = { 'exe': '$HOME/Ruby24/bin/ruby',
                    'inc': '$HOME/Ruby24/include/ruby-2.4.0',
                    'lib': '$HOME/Ruby24/lib/libruby.2.4.dylib'
                  }

# Consolidated dictionary kit for Ruby
RubyDictionary  = { 'nil'           : None,
                    'RubyYosemite'  : RubyYosemite,
                    'RubyElCapitan' : RubyElCapitan,
                    'RubySierra'    : RubySierra,
                    'RubyHighSierra': RubyHighSierra,
                    'Ruby24SrcBuild': Ruby24SrcBuild
                  }

#-----------------------------------------------------
# [3] Python
#-----------------------------------------------------
Pythons  = [ 'nil', 'PythonYosemite', 'PythonElCapitan', 'PythonSierra', 'PythonHighSierra' ]
Pythons += [ 'Anaconda27', 'Anaconda36' ]

#-----------------------------------------------------
# Whereabout of different components of Python
#-----------------------------------------------------
# Bundled with Yosemite (10.10)
PythonYosemite  = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with El Capitan (10.11)
PythonElCapitan = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with Sierra (10.12)
PythonSierra    = { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Bundled with High Sierra (10.13)
PythonHighSierra= { 'exe': '/System/Library/Frameworks/Python.framework/Versions/2.7/bin/python' ,
                    'inc': '/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7',
                    'lib': '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib'
                  }

# Using anaconda (https://www.anaconda.com/download/#macos): *+*+*+ EXPERIMENTAL *+*+*+
#   If the path to your `conda` command is '$HOME/anaconda/bin/conda'
#   and your Python environment was prepared by: $ conda create -n py27klayout python=2.7
#
#   No additional modules are added in the beginning.
Anaconda27      = { 'exe': '$HOME/anaconda/envs/py27klayout/bin/python2.7' ,
                    'inc': '$HOME/anaconda/envs/py27klayout/include/python2.7',
                    'lib': '$HOME/anaconda/envs/py27klayout/lib/libpython2.7.dylib'
                  }

# Using anaconda (https://www.anaconda.com/download/#macos): *+*+*+ EXPERIMENTAL *+*+*+
#   If the path to your `conda` command is '$HOME/anaconda/bin/conda'
#   and your Python environment was prepared by: $ conda create -n py36klayout python=3.6
#
#   No additional modules are added in the beginning.
Anaconda36      = { 'exe': '$HOME/anaconda/envs/py36klayout/bin/python3.6' ,
                    'inc': '$HOME/anaconda/envs/py36klayout/include/python3.6m',
                    'lib': '$HOME/anaconda/envs/py36klayout/lib/libpython3.6m.dylib'
                  }

# Consolidated dictionary kit for Python
PythonDictionary= { 'nil'             : None,
                    'PythonYosemite'  : PythonYosemite,
                    'PythonElCapitan' : PythonElCapitan,
                    'PythonSierra'    : PythonSierra,
                    'PythonHighSierra': PythonHighSierra,
                    'Anaconda27'      : Anaconda27,
                    'Anaconda36'      : Anaconda36
                  }

#-----------------------------------------------------
# [4] KLayout executables
#-----------------------------------------------------
KLayoutExecs  = ['klayout']
KLayoutExecs += ['strm2cif', 'strm2dxf', 'strm2gds', 'strm2gdstxt', 'strm2oas']
KLayoutExecs += ['strm2txt', 'strmclip', 'strmcmp',  'strmrun',     'strmxor']

#----------------
# End of File
#----------------
