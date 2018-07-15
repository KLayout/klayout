
"""

KLayout standalone Python module setup script


  Copyright (C) 2006-2018 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


This script provides distutils-based deployment for KLayout's
standalone Python modules.

The standalone libraries are basically extension modules.
Build requirements are:
 * curl library
 * expat library

The main challenge is to map KLayout's shared object architecture.
The structure consists of the Python extension libraries and a bunch
of libraries providing the actual implementation part. 
The Python extension libraries reference the implementation libraries.
The extension libraries are prefixed with an underscore.

For example:

  tl (Python) -> _tl, _gsi

There is a specific issue with the building of the extension 
libraries: distutils only allows building of shared objects
as extension libraries and does not provide a intrinsic feature
for linking other libraries into the extension library. 

Hence we need to add the dependencies through "extra_objects".

On Linux there is a specific issue: The shared objects produced 
for extension libraries are not matching the Linux name conventions.
So we have to add them as .so link objects including the path. 
If we do so, the runtime loading will look them up by their path 
and won't find them. So we need to take away the path with 
"-Wl,-soname" on Linux (see Config.link_args).
"""

from distutils.core import setup, Extension, Distribution
import glob
import os
import platform
import sysconfig

# ----------------------------------------------------------------------------------------

class Config(object):

  """
  Provides some configuration-specific methods
  """

  def __init__(self):

    # TODO: is this really how we get the build paths?

    build_cmd = Distribution().get_command_obj('build')
    build_cmd.finalize_options()
    self.build_platlib = build_cmd.build_platlib

    install_cmd = Distribution().get_command_obj('install')
    install_cmd.finalize_options()
    self.install_platlib = install_cmd.install_platlib

    self.ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")
    self.root = "klayout"

  def libname_of(self, mod):
    """
    Returns the library name for a given module
    The library name is usually decorated (i.e. "tl" -> "tl.cpython-35m-x86_64-linux-gnu.so").
    """
    return mod + self.ext_suffix

  def path_of(self, mod):
    """
    Returns the build path of the library for a given module
    """
    return os.path.join(self.build_platlib, self.root, self.libname_of(mod))

  def rpath(self):
    """
    Returns the runtime_library_dir to use when linking the modules
    This path will ensure the auxiliary modules are found at runtime.
    """
    return [ os.path.join(self.install_platlib, self.root) ]

  def compile_args(self, mod):
    """
    Gets additional compiler arguments
    """
    if platform.system() == "Windows":
      return [ ]
    elif platform.system() == "Darwin":
      return [ ]
    else:
      # Avoids many "type-punned pointer" warnings
      return [ "-Wno-strict-aliasing" ]

  def link_args(self, mod):
    """
    Gets additional linker arguments
    """
    if platform.system() == "Windows":
      return [ ]
    elif platform.system() == "Darwin":
      # For the dependency modules, make sure we produce a dylib.
      # We can only link against such, but the bundles produced otherwise.
      if mod[0:1] == "_":
        return [ "-Wl,-dylib" ]
      else:
        return []
    else:
      # this makes the libraries suitable for linking with a path -
      # i.e. from path_of('_tl'). Without this option, the path
      # will be included in the reference and at runtime the loaded
      # will look for the path-qualified library. But that's the 
      # build path and the loader will fail.
      return ['-Wl,-soname,' + self.libname_of(mod)]

  def macros(self):
    """
    Returns the macros to use for building
    """
    return [ ('HAVE_CURL', 1), ('HAVE_EXPAT', 1) ]

  def version(self):
    """
    Gets the version string
    """
    return "0.26"

config = Config()

# ------------------------------------------------------------------
# _tl dependency library

_tl_sources = glob.glob("src/tl/tl/*.cc")

# Exclude sources which are compatible with Qt only
_tl_sources.remove("src/tl/tl/tlHttpStreamQt.cc")
_tl_sources.remove("src/tl/tl/tlHttpStreamNoQt.cc")
_tl_sources.remove("src/tl/tl/tlFileSystemWatcher.cc")
_tl_sources.remove("src/tl/tl/tlDeferredExecutionQt.cc")

_tl = Extension(config.root + '._tl', 
                define_macros = config.macros() + [ ('MAKE_TL_LIBRARY', 1) ],
                language = 'c++',
                libraries = [ 'curl', 'expat' ],
                extra_link_args = config.link_args('_tl'),
                extra_compile_args = config.compile_args('_tl'),
                sources = _tl_sources)

# ------------------------------------------------------------------
# _gsi dependency library

_gsi_sources = glob.glob("src/gsi/gsi/*.cc")

_gsi = Extension(config.root + '._gsi', 
                define_macros = config.macros() + [ ('MAKE_GSI_LIBRARY', 1) ],
                include_dirs = [ 'src/tl/tl' ],
                extra_objects = [ config.path_of('_tl') ],
                runtime_library_dirs = config.rpath(),
                language = 'c++',
                extra_link_args = config.link_args('_gsi'),
                extra_compile_args = config.compile_args('_gsi'),
                sources = _gsi_sources)

# ------------------------------------------------------------------
# _pya dependency library

_pya_sources = glob.glob("src/pya/pya/*.cc")

_pya = Extension(config.root + '._pya', 
                define_macros = config.macros() + [ ('MAKE_PYA_LIBRARY', 1) ],
                include_dirs = [ 'src/tl/tl', 'src/gsi/gsi' ],
                extra_objects = [ config.path_of('_tl'), config.path_of('_gsi') ],
                runtime_library_dirs = config.rpath(),
                language = 'c++',
                extra_link_args = config.link_args('_pya'),
                extra_compile_args = config.compile_args('_pya'),
                sources = _pya_sources)

# ------------------------------------------------------------------
# _db dependency library

_db_sources = glob.glob("src/db/db/*.cc")

# Not a real source:
_db_sources.remove("src/db/db/fonts.cc")

_db = Extension(config.root + '._db', 
                define_macros = config.macros() + [ ('MAKE_DB_LIBRARY', 1) ],
                include_dirs = [ 'src/tl/tl', 'src/gsi/gsi', 'src/db/db' ],
                extra_objects = [ config.path_of('_tl'), config.path_of('_gsi') ],
                runtime_library_dirs = config.rpath(),
                language = 'c++',
                extra_link_args = config.link_args('_db'),
                extra_compile_args = config.compile_args('_db'),
                sources = _db_sources)

# ------------------------------------------------------------------
# _rdb dependency library

_rdb_sources = glob.glob("src/rdb/rdb/*.cc")

_rdb = Extension(config.root + '._rdb', 
                define_macros = config.macros() + [ ('MAKE_RDB_LIBRARY', 1) ],
                include_dirs = [ 'src/db/db', 'src/tl/tl', 'src/gsi/gsi' ],
                extra_objects = [ config.path_of('_tl'), config.path_of('_gsi'), config.path_of('_db') ],
                runtime_library_dirs = config.rpath(),
                language = 'c++',
                extra_link_args = config.link_args('_rdb'),
                extra_compile_args = config.compile_args('_rdb'),
                sources = _rdb_sources)

# ------------------------------------------------------------------
# dependency libraries from db_plugins

db_plugins = []

for pi in glob.glob("src/plugins/*/db_plugin") + glob.glob("src/plugins/*/*/db_plugin"):

  mod_name = "_" + os.path.split(os.path.split(pi)[-2])[-1] + "_dbpi"

  pi_sources = glob.glob(pi + "/*.cc")

  pi_ext = Extension(config.root + '.db_plugins.' + mod_name,
                define_macros = config.macros() + [ ('MAKE_DB_PLUGIN_LIBRARY', 1) ],
                include_dirs = [ 'src/plugins/common', 'src/db/db', 'src/tl/tl', 'src/gsi/gsi' ],
                extra_objects = [ config.path_of('_tl'), config.path_of('_gsi'), config.path_of('_db') ],
                runtime_library_dirs = config.rpath(),
                language = 'c++',
                extra_link_args = config.link_args(mod_name),
                extra_compile_args = config.compile_args(mod_name),
                sources = pi_sources)
                     
  db_plugins.append(pi_ext)
  
# ------------------------------------------------------------------
# tl extension library

tl_sources = glob.glob("src/pymod/tl/*.cc")

tl = Extension(config.root + '.tl', 
                define_macros = config.macros(),
                include_dirs = [ 'src/tl/tl', 'src/gsi/gsi', 'src/pya/pya' ],
                extra_objects = [ config.path_of('_tl'), config.path_of('_gsi'), config.path_of('_pya') ],
                runtime_library_dirs = config.rpath(),
                sources = tl_sources)

# ------------------------------------------------------------------
# db extension library

db_sources = glob.glob("src/pymod/db/*.cc")

db = Extension(config.root + '.db', 
                define_macros = config.macros(),
                include_dirs = [ 'src/db/db', 'src/tl/tl', 'src/gsi/gsi', 'src/pya/pya' ],
                extra_objects = [ config.path_of('_db'), config.path_of('_tl'), config.path_of('_gsi'), config.path_of('_pya') ],
                runtime_library_dirs = config.rpath(),
                sources = db_sources)

# ------------------------------------------------------------------
# rdb extension library

rdb_sources = glob.glob("src/pymod/rdb/*.cc")

rdb = Extension(config.root + '.rdb', 
                define_macros = config.macros(),
                include_dirs = [ 'src/rdb/rdb', 'src/db/db', 'src/tl/tl', 'src/gsi/gsi', 'src/pya/pya' ],
                extra_objects = [ config.path_of('_rdb'), config.path_of('_gsi'), config.path_of('_pya') ],
                runtime_library_dirs = config.rpath(),
                sources = rdb_sources)

# ------------------------------------------------------------------
# Core setup function

setup(name = config.root,
      version = config.version(),
      description = 'KLayout standalone Python package',
      author = 'Matthias Koefferlein',
      author_email = 'matthias@klayout.de',
      packages = [ config.root ],
      package_dir = { config.root: 'src/pymod/distutils_src' },
      ext_modules = [ _tl, _gsi, _pya, _db, _rdb ] + db_plugins + [ tl, db, rdb ])

