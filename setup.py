
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

from setuptools import setup, Extension, Distribution
import glob
import os
import platform
import distutils.sysconfig as sysconfig
from distutils.errors import CompileError
import multiprocessing
N_cores = multiprocessing.cpu_count()


# monkey-patch for parallel compilation
# from https://stackoverflow.com/questions/11013851/speeding-up-build-process-with-distutils
def parallelCCompile(self, sources, output_dir=None, macros=None, include_dirs=None, debug=0, extra_preargs=None, extra_postargs=None, depends=None):
    # those lines are copied from distutils.ccompiler.CCompiler directly
    macros, objects, extra_postargs, pp_opts, build = self._setup_compile(
        output_dir, macros, include_dirs, sources, depends, extra_postargs)
    cc_args = self._get_cc_args(pp_opts, debug, extra_preargs)
    # parallel code

    N = min(N_cores, len(objects) // 2, 8)  # number of parallel compilations
    N = max(N, 1)
    print("Compiling with", N, "threads.")
    import multiprocessing.pool

    def _single_compile(obj):
        try:
            src, ext = build[obj]
        except KeyError:
            return
        n_tries = 2
        while n_tries > 0:
            try:
                self._compile(obj, src, ext, cc_args, extra_postargs, pp_opts)
            except CompileError:
                n_tries -= 1
                print("Building", obj, "has failed. Trying again.")
            else:
                break
    # convert to list, imap is evaluated on-demand
    list(multiprocessing.pool.ThreadPool(N).imap(_single_compile, objects))
    return objects


# only if python version > 2.6, somehow the travis compiler hangs in 2.6
import sys
if sys.version_info[0] * 10 + sys.version_info[1] > 26:
    import distutils.ccompiler
    distutils.ccompiler.CCompiler.compile = parallelCCompile

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

        self.ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")
        if self.ext_suffix is None:
            self.ext_suffix = ".so"

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

    def compile_args(self, mod):
        """
        Gets additional compiler arguments
        """
        if platform.system() == "Windows":
            return []
        elif platform.system() == "Darwin":
            return []
        else:
            return ["-Wno-strict-aliasing",  # Avoids many "type-punned pointer" warnings
                    "-std=c++0x",  # because we use unordered_map/unordered_set
                    ]

    def link_args(self, mod):
        """
        Gets additional linker arguments
        """
        if platform.system() == "Windows":
            return []
        elif platform.system() == "Darwin":
            # For the dependency modules, make sure we produce a dylib.
            # We can only link against such, but the bundles produced otherwise.
            args = []
            if mod[0] == "_":
                args += ["-Wl,-dylib", '-Wl,-install_name,@rpath/%s' % self.libname_of(mod)]
            args += ['-Wl,-rpath,@loader_path/']
            return args
        else:
            # this makes the libraries suitable for linking with a path -
            # i.e. from path_of('_tl'). Without this option, the path
            # will be included in the reference and at runtime the loaded
            # will look for the path-qualified library. But that's the
            # build path and the loader will fail.
            args = []
            args += ['-Wl,-soname,' + self.libname_of(mod)]
            if '_dbpi' not in mod:
                loader_path = '$ORIGIN'
            else:
                loader_path = '$ORIGIN/..'
            args += ['-Wl,-rpath,' + loader_path]
            return args

    def macros(self):
        """
        Returns the macros to use for building
        """
        return [('HAVE_CURL', 1), ('HAVE_EXPAT', 1)]

    def version(self):
        """
        Gets the version string
        """
        return "0.26.0.dev2"


config = Config()

# ------------------------------------------------------------------
# _tl dependency library

_tl_path = os.path.join("src", "tl", "tl")

_tl_sources = set(glob.glob(os.path.join(_tl_path, "*.cc")))

# Exclude sources which are compatible with Qt only
# Caveat, in source distribution tarballs from pypi, these files will
# not exist. So we need an error-free discard method instead of list's remove.
_tl_sources.discard(os.path.join(_tl_path, "tlHttpStreamQt.cc"))
_tl_sources.discard(os.path.join(_tl_path, "tlHttpStreamNoQt.cc"))
_tl_sources.discard(os.path.join(_tl_path, "tlFileSystemWatcher.cc"))
_tl_sources.discard(os.path.join(_tl_path, "tlDeferredExecutionQt.cc"))

_tl = Extension(config.root + '._tl',
                define_macros=config.macros() + [('MAKE_TL_LIBRARY', 1)],
                language='c++',
                libraries=['curl', 'expat'],
                extra_link_args=config.link_args('_tl'),
                extra_compile_args=config.compile_args('_tl'),
                sources=list(_tl_sources))

# ------------------------------------------------------------------
# _gsi dependency library

_gsi_sources = glob.glob("src/gsi/gsi/*.cc")

_gsi = Extension(config.root + '._gsi',
                 define_macros=config.macros() + [('MAKE_GSI_LIBRARY', 1)],
                 include_dirs=['src/tl/tl'],
                 extra_objects=[config.path_of('_tl')],
                 language='c++',
                 extra_link_args=config.link_args('_gsi'),
                 extra_compile_args=config.compile_args('_gsi'),
                 sources=_gsi_sources)

# ------------------------------------------------------------------
# _pya dependency library

_pya_sources = glob.glob("src/pya/pya/*.cc")

_pya = Extension(config.root + '._pya',
                 define_macros=config.macros() + [('MAKE_PYA_LIBRARY', 1)],
                 include_dirs=['src/tl/tl', 'src/gsi/gsi'],
                 extra_objects=[config.path_of('_tl'), config.path_of('_gsi')],
                 language='c++',
                 extra_link_args=config.link_args('_pya'),
                 extra_compile_args=config.compile_args('_pya'),
                 sources=_pya_sources)

# ------------------------------------------------------------------
# _db dependency library

_db_path = os.path.join("src", "db", "db")
_db_sources = set(glob.glob(os.path.join(_db_path, "*.cc")))

# Not a real source:
# Caveat, in source distribution tarballs from pypi, these files will
# not exist. So we need an error-free discard method instead of list's remove.
_db_sources.discard(os.path.join(_db_path, "fonts.cc"))

_db = Extension(config.root + '._db',
                define_macros=config.macros() + [('MAKE_DB_LIBRARY', 1)],
                include_dirs=['src/tl/tl', 'src/gsi/gsi', 'src/db/db'],
                extra_objects=[config.path_of('_tl'), config.path_of('_gsi')],
                language='c++',
                extra_link_args=config.link_args('_db'),
                extra_compile_args=config.compile_args('_db'),
                sources=list(_db_sources))

# ------------------------------------------------------------------
# _rdb dependency library

_rdb_sources = glob.glob("src/rdb/rdb/*.cc")

_rdb = Extension(config.root + '._rdb',
                 define_macros=config.macros() + [('MAKE_RDB_LIBRARY', 1)],
                 include_dirs=['src/db/db', 'src/tl/tl', 'src/gsi/gsi'],
                 extra_objects=[config.path_of('_tl'), config.path_of(
                     '_gsi'), config.path_of('_db')],
                 language='c++',
                 extra_link_args=config.link_args('_rdb'),
                 extra_compile_args=config.compile_args('_rdb'),
                 sources=_rdb_sources)

# ------------------------------------------------------------------
# dependency libraries from db_plugins

db_plugins = []

for pi in glob.glob("src/plugins/*/db_plugin") + glob.glob("src/plugins/*/*/db_plugin"):

    mod_name = "_" + os.path.split(os.path.split(pi)[-2])[-1] + "_dbpi"

    pi_sources = glob.glob(pi + "/*.cc")

    pi_ext = Extension(config.root + '.db_plugins.' + mod_name,
                       define_macros=config.macros() + [('MAKE_DB_PLUGIN_LIBRARY', 1)],
                       include_dirs=['src/plugins/common',
                                     'src/db/db', 'src/tl/tl', 'src/gsi/gsi'],
                       extra_objects=[config.path_of('_tl'), config.path_of(
                           '_gsi'), config.path_of('_db')],
                       language='c++',
                       extra_link_args=config.link_args(mod_name),
                       extra_compile_args=config.compile_args(mod_name),
                       sources=pi_sources)

    db_plugins.append(pi_ext)

# ------------------------------------------------------------------
# tl extension library

tl_sources = glob.glob("src/pymod/tl/*.cc")

tl = Extension(config.root + '.tl',
               define_macros=config.macros(),
               include_dirs=['src/tl/tl', 'src/gsi/gsi', 'src/pya/pya'],
               extra_objects=[config.path_of('_tl'), config.path_of(
                   '_gsi'), config.path_of('_pya')],
               extra_link_args=config.link_args('tl'),
               sources=tl_sources)

# ------------------------------------------------------------------
# db extension library

db_sources = glob.glob("src/pymod/db/*.cc")

db = Extension(config.root + '.db',
               define_macros=config.macros(),
               include_dirs=['src/db/db', 'src/tl/tl', 'src/gsi/gsi', 'src/pya/pya'],
               extra_objects=[config.path_of('_db'), config.path_of(
                   '_tl'), config.path_of('_gsi'), config.path_of('_pya')],
               extra_link_args=config.link_args('db'),
               sources=db_sources)

# ------------------------------------------------------------------
# rdb extension library

rdb_sources = glob.glob("src/pymod/rdb/*.cc")

rdb = Extension(config.root + '.rdb',
                define_macros=config.macros(),
                include_dirs=['src/rdb/rdb', 'src/db/db',
                              'src/tl/tl', 'src/gsi/gsi', 'src/pya/pya'],
                extra_objects=[config.path_of('_rdb'), config.path_of(
                    '_gsi'), config.path_of('_pya')],
                extra_link_args=config.link_args('rdb'),
                sources=rdb_sources)

# ------------------------------------------------------------------
# Core setup function

if __name__ == '__main__':
    setup(name=config.root,
          version=config.version(),
          license='GNU GPLv3',
          description='KLayout standalone Python package',
          long_description='TODO',
          author='Matthias Koefferlein',
          author_email='matthias@klayout.de',
          url='https://github.com/klayoutmatthias/klayout',
          packages=[config.root],
          package_dir={config.root: 'src/pymod/distutils_src'},
          ext_modules=[_tl, _gsi, _pya, _db, _rdb] + db_plugins + [tl, db, rdb])
