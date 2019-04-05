
"""

KLayout standalone Python module setup script


    Copyright (C) 2006-2019 Matthias Koefferlein

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

from setuptools import setup, Distribution, find_packages
from setuptools.extension import Extension, Library
import glob
import os
import platform
from distutils.errors import CompileError
import distutils.command.build_ext
import setuptools.command.build_ext
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
                print("Building", obj)
                self._compile(obj, src, ext, cc_args, extra_postargs, pp_opts)
            except CompileError:
                n_tries -= 1
                print("Building", obj, "has failed. Trying again.")
            else:
                break
    # convert to list, imap is evaluated on-demand
    list(multiprocessing.pool.ThreadPool(N).map(_single_compile, objects))
    return objects


# only if python version > 2.6, somehow the travis compiler hangs in 2.6
import sys
if sys.version_info[0] * 10 + sys.version_info[1] > 26:
    import distutils.ccompiler
    distutils.ccompiler.CCompiler.compile = parallelCCompile


# TODO: delete (Obsolete)
# patch get_ext_filename
from distutils.command.build_ext import build_ext
_old_get_ext_filename = build_ext.get_ext_filename


def patched_get_ext_filename(self, ext_name):
    r"""Convert the name of an extension (eg. "foo.bar") into the name
    of the file from which it will be loaded (eg. "foo/bar.so", or
    "foo\bar.pyd").
    """
    filename = _old_get_ext_filename(self, ext_name)
    # Making sure this matches qmake's default extension .dylib, instead of .so
    if platform.system() == "Darwin" and '_dbpi' in ext_name:
        filename = filename.replace('.so', '.dylib')
    return filename


distutils.command.build_ext.build_ext.get_ext_filename = patched_get_ext_filename

# end patch get_ext_filename


# patch CCompiler's library_filename for _dbpi libraries (SOs)
# Its default is to have .so for shared objects, instead of dylib,
# hence the patch

from distutils.ccompiler import CCompiler
old_library_filename = CCompiler.library_filename


def patched_library_filename(self, libname, lib_type='static',     # or 'shared'
                     strip_dir=0, output_dir=''):
    if platform.system() == "Darwin" and '_dbpi' in libname:
        lib_type = 'dylib'
    return old_library_filename(self, libname, lib_type=lib_type,
        strip_dir=strip_dir, output_dir=output_dir)


distutils.ccompiler.CCompiler.library_filename = patched_library_filename

# end patch CCompiler's library_filename for _dbpi libraries (SOs)


# despite its name, setuptools.command.build_ext.link_shared_object won't
# link a shared object on Linux, but a static library and patches distutils
# for this ... We're patching this back now.

def always_link_shared_object(
        self, objects, output_libname, output_dir=None, libraries=None,
        library_dirs=None, runtime_library_dirs=None, export_symbols=None,
        debug=0, extra_preargs=None, extra_postargs=None, build_temp=None,
        target_lang=None):
    self.link(
        self.SHARED_LIBRARY, objects, output_libname,
        output_dir, libraries, library_dirs, runtime_library_dirs,
        export_symbols, debug, extra_preargs, extra_postargs,
        build_temp, target_lang
    )


setuptools.command.build_ext.libtype = "shared"
setuptools.command.build_ext.link_shared_object = always_link_shared_object

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
        self.build_temp = build_cmd.build_temp
        if platform.system() == "Windows":
            # Windows uses separate temp build folders for debug and release
            if build_cmd.debug:
                self.build_temp = os.path.join(self.build_temp, "Debug")
            else:
                self.build_temp = os.path.join(self.build_temp, "Release")

        build_ext_cmd = Distribution().get_command_obj('build_ext')

        build_ext_cmd.initialize_options()
        build_ext_cmd.setup_shlib_compiler()
        self.build_ext_cmd = build_ext_cmd

        self.root = "klayout"

    def add_extension(self, ext):
        self.build_ext_cmd.ext_map[ext.name] = ext

    def libname_of(self, mod, is_lib=None):
        """
        Returns the library name for a given module
        The library name is usually decorated (i.e. "tl" -> "tl.cpython-35m-x86_64-linux-gnu.so").
        This code was extracted from the source in setuptools.command.build_ext.build_ext
        """
        libtype = setuptools.command.build_ext.libtype
        full_mod = self.root + '.' + mod
        if is_lib is True:
            # Here we are guessing it is a library and that the filename
            # is to be computed by shlib_compiler.
            # See source code of setuptools.command.build_ext.build_ext.get_ext_filename
            filename = self.build_ext_cmd.get_ext_filename(full_mod)
            fn, ext = os.path.splitext(filename)
            ext_path = self.build_ext_cmd.shlib_compiler.library_filename(fn, libtype)
        else:
            # This assumes that the Extension/Library object was added to the
            # ext_map dictionary via config.add_extension.
            assert full_mod in self.build_ext_cmd.ext_map
            ext_path = self.build_ext_cmd.get_ext_filename(full_mod)
        ext_filename = os.path.basename(ext_path)

        # Exception for database plugins, which will always be dylib.
        if platform.system() == "Darwin" and '_dbpi' in mod:
            ext_filename = ext_filename.replace('.so', '.dylib')
        return ext_filename

    def path_of(self, mod, mod_src_path):
        """
        Returns the build path of the library for a given module
        """
        if platform.system() == "Windows":
            # On Windows, the library to link is the import library
            (dll_name, dll_ext) = os.path.splitext(self.libname_of(mod))
            return os.path.join(self.build_temp, mod_src_path, dll_name + ".lib")
        else:
            return os.path.join(self.build_platlib, self.root, self.libname_of(mod))

    def compile_args(self, mod):
        """
        Gets additional compiler arguments
        """
        if platform.system() == "Windows":
            bits = os.getenv("KLAYOUT_BITS")
            if bits:
                return ["\"-I" + os.path.join(bits, "zlib", "include") + "\"",
                        "\"-I" + os.path.join(bits, "ptw", "include") + "\"",
                        "\"-I" + os.path.join(bits, "expat", "include") + "\"",
                        "\"-I" + os.path.join(bits, "curl", "include") + "\""]
            else:
                return []
        elif platform.system() == "Darwin":
            return []
        else:
            return ["-Wno-strict-aliasing",  # Avoids many "type-punned pointer" warnings
                    "-std=c++11",  # because we use unordered_map/unordered_set
                    ]

    def libraries(self, mod):
        """
        Gets the libraries to add
        """
        if platform.system() == "Windows":
            if mod == "_tl":
                return [ "libcurl", "expat", "pthreadVCE2", "zlib", "wsock32" ]
        else:
            if mod == "_tl":
                return ['curl', 'expat']
        return []

    def link_args(self, mod):
        """
        Gets additional linker arguments
        """
        if platform.system() == "Windows":
            args = ["/DLL"]
            bits = os.getenv("KLAYOUT_BITS")
            if bits:
                args += ["\"/LIBPATH:" + os.path.join(bits, "zlib", "libraries") + "\"",
                         "\"/LIBPATH:" + os.path.join(bits, "ptw", "libraries") + "\"",
                         "\"/LIBPATH:" + os.path.join(bits, "expat", "libraries") + "\"",
                         "\"/LIBPATH:" + os.path.join(bits, "curl", "libraries") + "\""]
            return args
        elif platform.system() == "Darwin":
            # For the dependency modules, make sure we produce a dylib.
            # We can only link against such, but the bundles produced otherwise.
            args = []
            if mod[0] == "_":
                args += ["-Wl,-dylib", '-Wl,-install_name,@rpath/%s' % self.libname_of(mod, is_lib=True)]
            args += ['-Wl,-rpath,@loader_path/']
            return args
        else:
            # this makes the libraries suitable for linking with a path -
            # i.e. from path_of('_tl'). Without this option, the path
            # will be included in the reference and at runtime the loaded
            # will look for the path-qualified library. But that's the
            # build path and the loader will fail.
            args = []
            args += ['-Wl,-soname,' + self.libname_of(mod, is_lib=True)]
            if '_dbpi' not in mod:
                loader_path = '$ORIGIN'
            else:
                loader_path = '$ORIGIN/..'
            args += ['-Wl,-rpath,' + loader_path]
            # default linux shared object compilation uses the '-g' flag,
            # which generates unnecessary debug information
            # removing with strip-all during the linking stage
            args += ['-Wl,--strip-all']
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
        return "0.26.0.dev15"


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

_tl = Library(config.root + '._tl',
              define_macros=config.macros() + [('MAKE_TL_LIBRARY', 1)],
              language='c++',
              libraries=config.libraries('_tl'),
              extra_link_args=config.link_args('_tl'),
              extra_compile_args=config.compile_args('_tl'),
              sources=list(_tl_sources))

config.add_extension(_tl)

# ------------------------------------------------------------------
# _gsi dependency library

_gsi_path = os.path.join("src", "gsi", "gsi")
_gsi_sources = set(glob.glob(os.path.join(_gsi_path, "*.cc")))

_gsi = Library(config.root + '._gsi',
               define_macros=config.macros() + [('MAKE_GSI_LIBRARY', 1)],
               include_dirs=[_tl_path],
               extra_objects=[config.path_of('_tl', _tl_path)],
               language='c++',
               extra_link_args=config.link_args('_gsi'),
               extra_compile_args=config.compile_args('_gsi'),
               sources=list(_gsi_sources))
config.add_extension(_gsi)

# ------------------------------------------------------------------
# _pya dependency library

_pya_path = os.path.join("src", "pya", "pya")
_pya_sources = set(glob.glob(os.path.join(_pya_path, "*.cc")))

_pya = Library(config.root + '._pya',
               define_macros=config.macros() + [('MAKE_PYA_LIBRARY', 1)],
               include_dirs=[_tl_path, _gsi_path],
               extra_objects=[config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path)],
               language='c++',
               extra_link_args=config.link_args('_pya'),
               extra_compile_args=config.compile_args('_pya'),
               sources=list(_pya_sources))
config.add_extension(_pya)

# ------------------------------------------------------------------
# _db dependency library

_db_path = os.path.join("src", "db", "db")
_db_sources = set(glob.glob(os.path.join(_db_path, "*.cc")))

_db = Library(config.root + '._db',
              define_macros=config.macros() + [('MAKE_DB_LIBRARY', 1)],
              include_dirs=[_tl_path, _gsi_path, _db_path],
              extra_objects=[config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path)],
              language='c++',
              extra_link_args=config.link_args('_db'),
              extra_compile_args=config.compile_args('_db'),
              sources=list(_db_sources))
config.add_extension(_db)

# ------------------------------------------------------------------
# _lib dependency library

_lib_path = os.path.join("src", "lib", "lib")
_lib_sources = set(glob.glob(os.path.join(_lib_path, "*.cc")))

_lib = Library(config.root + '._lib',
              define_macros=config.macros() + [('MAKE_LIB_LIBRARY', 1)],
              include_dirs=[_tl_path, _gsi_path, _db_path, _lib_path],
              extra_objects=[config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path), config.path_of('_db', _db_path)],
              language='c++',
              extra_link_args=config.link_args('_lib'),
              extra_compile_args=config.compile_args('_lib'),
              sources=list(_lib_sources))
config.add_extension(_lib)

# ------------------------------------------------------------------
# _rdb dependency library

_rdb_path = os.path.join("src", "rdb", "rdb")
_rdb_sources = set(glob.glob(os.path.join(_rdb_path, "*.cc")))

_rdb = Library(config.root + '._rdb',
               define_macros=config.macros() + [('MAKE_RDB_LIBRARY', 1)],
               include_dirs=[_db_path, _tl_path, _gsi_path],
               extra_objects=[config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path), config.path_of('_db', _db_path)],
               language='c++',
               extra_link_args=config.link_args('_rdb'),
               extra_compile_args=config.compile_args('_rdb'),
               sources=list(_rdb_sources))
config.add_extension(_rdb)

# ------------------------------------------------------------------
# dependency libraries from db_plugins

db_plugins = []

dbpi_dirs = glob.glob(os.path.join("src", "plugins", "*", "db_plugin"))
dbpi_dirs += glob.glob(os.path.join("src", "plugins", "*", "*", "db_plugin"))

for pi in dbpi_dirs:

    mod_name = "_" + os.path.split(os.path.split(pi)[-2])[-1] + "_dbpi"

    pi_sources = glob.glob(os.path.join(pi, "*.cc"))

    pi_ext = Library(config.root + '.db_plugins.' + mod_name,
                     define_macros=config.macros() + [('MAKE_DB_PLUGIN_LIBRARY', 1)],
                     include_dirs=[os.path.join("src", "plugins", "common"),
                                   _db_path, _tl_path, _gsi_path],
                     extra_objects=[config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path), config.path_of('_db', _db_path)],
                     language='c++',
                     extra_link_args=config.link_args(mod_name),
                     extra_compile_args=config.compile_args(mod_name),
                     sources=pi_sources)

    db_plugins.append(pi_ext)
    config.add_extension(pi_ext)

# ------------------------------------------------------------------
# tl extension library

tl_path = os.path.join("src", "pymod", "tl")
tl_sources = set(glob.glob(os.path.join(tl_path, "*.cc")))

tl = Extension(config.root + '.tlcore',
               define_macros=config.macros(),
               include_dirs=[_tl_path, _gsi_path, _pya_path],
               extra_objects=[config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path), config.path_of('_pya', _pya_path)],
               extra_link_args=config.link_args('tlcore'),
               sources=list(tl_sources))

# ------------------------------------------------------------------
# db extension library

db_path = os.path.join("src", "pymod", "db")
db_sources = set(glob.glob(os.path.join(db_path, "*.cc")))

db = Extension(config.root + '.dbcore',
               define_macros=config.macros(),
               include_dirs=[_db_path, _tl_path, _gsi_path, _pya_path],
               extra_objects=[config.path_of('_db', _db_path), config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path), config.path_of('_pya', _pya_path)],
               extra_link_args=config.link_args('dbcore'),
               sources=list(db_sources))

# ------------------------------------------------------------------
# lib extension library

lib_path = os.path.join("src", "pymod", "lib")
lib_sources = set(glob.glob(os.path.join(lib_path, "*.cc")))

lib = Extension(config.root + '.libcore',
               define_macros=config.macros(),
               include_dirs=[_lib_path, _tl_path, _gsi_path, _pya_path],
               extra_objects=[config.path_of('_lib', _lib_path), config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path), config.path_of('_pya', _pya_path)],
               extra_link_args=config.link_args('libcore'),
               sources=list(lib_sources))

# ------------------------------------------------------------------
# rdb extension library

rdb_path = os.path.join("src", "pymod", "rdb")
rdb_sources = set(glob.glob(os.path.join(rdb_path, "*.cc")))

rdb = Extension(config.root + '.rdbcore',
                define_macros=config.macros(),

                include_dirs=[_rdb_path, _tl_path, _gsi_path, _pya_path],
                extra_objects=[config.path_of('_rdb', _rdb_path), config.path_of('_tl', _tl_path), config.path_of('_gsi', _gsi_path), config.path_of('_pya', _pya_path)],
                extra_link_args=config.link_args('rdbcore'),
                sources=list(rdb_sources))

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
          classifiers=[
              # Recommended classifiers
              "Programming Language :: Python :: 2",
              "Programming Language :: Python :: 3",
              "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
              "Operating System :: MacOS :: MacOS X",
              "Operating System :: Microsoft :: Windows",
              "Operating System :: POSIX :: Linux",
              # Optional classifiers
              "Topic :: Scientific/Engineering :: Electronic Design Automation (EDA)",
          ],
          url='https://github.com/klayoutmatthias/klayout',
          packages=find_packages('src/pymod/distutils_src'),
          package_dir={'': 'src/pymod/distutils_src'},  # https://github.com/pypa/setuptools/issues/230
          ext_modules=[_tl, _gsi, _pya, _db, _lib, _rdb] + db_plugins + [tl, db, lib, rdb])
