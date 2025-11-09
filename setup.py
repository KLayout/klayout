"""

KLayout standalone Python module setup script


    Copyright (C) 2006-2025 Matthias Koefferlein

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
 * png library

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

from typing import List
from setuptools import setup, Distribution, find_packages
from setuptools.extension import Extension, Library
from pathlib import Path
import glob
import os
import re
import sys
import platform
from distutils.errors import CompileError
import distutils.command.build_ext
import setuptools.command.build_ext
from setuptools.command.build_ext import build_ext as _build_ext
import multiprocessing

# for Jenkins we do not want to be greedy
multicore = os.getenv("KLAYOUT_SETUP_MULTICORE")
if multicore:
    N_cores = int(multicore)
else:
    N_cores = multiprocessing.cpu_count()


# monkey-patch for parallel compilation
# from https://stackoverflow.com/questions/11013851/speeding-up-build-process-with-distutils
def parallelCCompile(
    self,
    sources,
    output_dir=None,
    macros=None,
    include_dirs=None,
    debug=0,
    extra_preargs=None,
    extra_postargs=None,
    depends=None,
):
    # those lines are copied from distutils.ccompiler.CCompiler directly
    macros, objects, extra_postargs, pp_opts, build = self._setup_compile(
        output_dir, macros, include_dirs, sources, depends, extra_postargs
    )
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
if sys.version_info[0] * 100 + sys.version_info[1] > 206:
    import distutils.ccompiler

    distutils.ccompiler.CCompiler.compile = parallelCCompile


# put a path in quotes if required
def quote_path(path):
    # looks like disutils don't need path quoting in version >= 3.9:
    if " " in path and sys.version_info[0] * 100 + sys.version_info[1] < 309:
        return '"' + path + '"'
    else:
        return path

import subprocess

def check_libpng():
    """ Check if libpng is available (Linux & Macos only)"""
    try:
        subprocess.check_call(["libpng-config", "--version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        return True
    except FileNotFoundError as e:
        # libpng is not present.
        if True: # change to False to enable installation without libpng
            raise RuntimeError("libpng missing. libpng-config is not available") from e
        return False

def libpng_cflags():
    return subprocess.check_output(["libpng-config", "--cflags"]).decode().split()

def libpng_ldflags():
    return subprocess.check_output(["libpng-config", "--ldflags"]).decode().split()

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
    if platform.system() == "Darwin" and "_dbpi" in ext_name:
        filename = filename.replace(".so", ".dylib")
    return filename


distutils.command.build_ext.build_ext.get_ext_filename = patched_get_ext_filename

# end patch get_ext_filename

# TODO: customize this object instead of introspecting and patching distutils (if possible)
class klayout_build_ext(_build_ext):
    """ Customize build extension class to check for dependencies before installing."""
    def finalize_options(self) -> None:
        ret = super().finalize_options()
        return ret

# patch CCompiler's library_filename for _dbpi libraries (SOs)
# Its default is to have .so for shared objects, instead of dylib,
# hence the patch

from distutils.ccompiler import CCompiler

old_library_filename = CCompiler.library_filename


def patched_library_filename(
    self, libname, lib_type="static", strip_dir=0, output_dir=""  # or 'shared'
):
    if platform.system() == "Darwin" and "_dbpi" in libname:
        lib_type = "dylib"
    return old_library_filename(
        self, libname, lib_type=lib_type, strip_dir=strip_dir, output_dir=output_dir
    )


distutils.ccompiler.CCompiler.library_filename = patched_library_filename

# end patch CCompiler's library_filename for _dbpi libraries (SOs)


# despite its name, setuptools.command.build_ext.link_shared_object won't
# link a shared object on Linux, but a static library and patches distutils
# for this ... We're patching this back now.


def always_link_shared_object(
    self,
    objects,
    output_libname,
    output_dir=None,
    libraries=None,
    library_dirs=None,
    runtime_library_dirs=None,
    export_symbols=None,
    debug=0,
    extra_preargs=None,
    extra_postargs=None,
    build_temp=None,
    target_lang=None,
):
    self.link(
        self.SHARED_LIBRARY,
        objects,
        output_libname,
        output_dir,
        libraries,
        library_dirs,
        runtime_library_dirs,
        export_symbols,
        debug,
        extra_preargs,
        extra_postargs,
        build_temp,
        target_lang,
    )

setuptools.command.build_ext.libtype = "shared"
setuptools.command.build_ext.link_shared_object = always_link_shared_object

# ----------------------------------------------------------------------------------------


# TODO: 
# * Pull this from module-specific configuration files?
# * Include dependencies: "extra_objects", "include_dirs"

class Config(object):

    """
    Provides some configuration-specific methods
    """

    def __init__(self):

        # TODO: is this really how we get the build paths?

        build_cmd = Distribution().get_command_obj("build")
        build_cmd.finalize_options()
        self.build_platlib = build_cmd.build_platlib
        self.build_temp = build_cmd.build_temp
        if platform.system() == "Windows":
            # Windows uses separate temp build folders for debug and release
            if build_cmd.debug:
                self.build_temp = os.path.join(self.build_temp, "Debug")
            else:
                self.build_temp = os.path.join(self.build_temp, "Release")

        build_ext_cmd = Distribution().get_command_obj("build_ext")

        build_ext_cmd.initialize_options()
        build_ext_cmd.setup_shlib_compiler()
        self.build_ext_cmd = build_ext_cmd

        self.root = "klayout"

        self.modules = { }

    def module(self, name, src_path):
        self.modules[name] = src_path

    def src_path(self, name):
        return self.modules[name]

    def add_extension(self, ext):
        self.build_ext_cmd.ext_map[ext.name] = ext

    def libname_of(self, mod, is_lib=None):
        """
        Returns the library name for a given module
        The library name is usually decorated (i.e. "tl" -> "tl.cpython-35m-x86_64-linux-gnu.so").
        This code was extracted from the source in setuptools.command.build_ext.build_ext
        """
        libtype = setuptools.command.build_ext.libtype
        full_mod = self.root + "." + mod
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
        if platform.system() == "Darwin" and "_dbpi" in mod:
            ext_filename = ext_filename.replace(".so", ".dylib")
        return ext_filename

    def path_of(self, mod):
        """
        Returns the build path of the library for a given module
        """
        if platform.system() == "Windows":
            # On Windows, the library to link is the import library
            (dll_name, dll_ext) = os.path.splitext(self.libname_of(mod))
            return os.path.join(self.build_temp, self.modules[mod], dll_name + ".lib")
        else:
            return os.path.join(self.build_platlib, self.root, self.libname_of(mod))

    def extra_include_dirs(self, mod):
        """
        Gets extra include directories per module
        """
        if mod == "_lstream_dbpi":
            mod_src = self.src_path(mod)
            return [ 
                os.path.join("src", "version"),  # does not use tlVersion.h
                os.path.join(mod_src, "capnp"),
                os.path.join(mod_src, "..", "runtime", "capnp"),
                os.path.join(mod_src, "..", "runtime", "kj")
            ]
        else:
            return []

    def sources(self, mod):
        """
        Gets the source files for the given module and root source path
        """
        mod_src = self.src_path(mod)
        if mod == "_gds2_dbpi":
            # GDS2 module has the contrib subfolder
            files = glob.glob(os.path.join(mod_src, "*.cc"))
            files += glob.glob(os.path.join(mod_src, "contrib", "*.cc"))
            return files
        elif mod == "_lstream_dbpi":
            # LStream module is .. complex
            files = glob.glob(os.path.join(mod_src, "*.cc"))
            files += glob.glob(os.path.join(mod_src, "capnp", "*.cc"))
            files += glob.glob(os.path.join(mod_src, "..", "runtime", "capnp", "capnp", "*.cc"))
            files += glob.glob(os.path.join(mod_src, "..", "runtime", "kj", "kj", "*.cc"))
            return files
        else:
            return glob.glob(os.path.join(mod_src, "*.cc"))
          
    def compile_args(self, mod):
        """
        Gets additional compiler arguments
        """
        args: List[str]
        if platform.system() == "Windows":
            bits = os.getenv("KLAYOUT_BITS")
            if bits:
                args = [
                    quote_path("-I" + os.path.join(bits, "zlib", "include")),
                    quote_path("-I" + os.path.join(bits, "ptw", "include")),
                    quote_path("-I" + os.path.join(bits, "png", "include")),
                    quote_path("-I" + os.path.join(bits, "expat", "include")),
                    quote_path("-I" + os.path.join(bits, "curl", "include")),
                ]
            else:
                args = []
        elif mod == "_lstream_dbpi":
            args = [
                "-std=c++14",  # cap'n'proto needs this
            ]
        else:
            args = [
                "-Wno-strict-aliasing",  # Avoids many "type-punned pointer" warnings
                "-std=c++11",  # because we use unordered_map/unordered_set
            ]
        if platform.system() == "Darwin" and mod == "_tl":
            if check_libpng():
                args += libpng_cflags()
        return args

    def libraries(self, mod):
        """
        Gets the libraries to add
        """
        if platform.system() == "Windows":
            if mod == "_tl":
                return ["libcurl", "expat", "pthreadVCE2", "zlib", "wsock32", "libpng16"]
        elif platform.system() == "Darwin":
            if mod == "_tl":
                libs = ["curl", "expat"]  # libpng is included by libpng_ldflags
                return libs
        else:
            if mod == "_tl":
                libs = ["curl", "expat", "png"]
                return libs
        return []

    def link_args(self, mod):
        """
        Gets additional linker arguments
        """
        if platform.system() == "Windows":
            args = ["/DLL"]
            bits = os.getenv("KLAYOUT_BITS")
            if bits:
                args += [
                    quote_path("/LIBPATH:" + os.path.join(bits, "zlib", "lib")),
                    quote_path("/LIBPATH:" + os.path.join(bits, "ptw", "libraries")),
                    quote_path("/LIBPATH:" + os.path.join(bits, "png", "libraries")),
                    quote_path("/LIBPATH:" + os.path.join(bits, "expat", "libraries")),
                    quote_path("/LIBPATH:" + os.path.join(bits, "curl", "libraries")),
                ]
            return args
        elif platform.system() == "Darwin":
            # For the dependency modules, make sure we produce a dylib.
            # We can only link against such, but the bundles produced otherwise.
            args = []
            if mod[0] == "_":
                args += [
                    "-Wl,-dylib",
                    "-Wl,-install_name,@rpath/%s" % self.libname_of(mod, is_lib=True),
                ]
            args += ["-Wl,-rpath,@loader_path/"]
            if mod == "_tl" and check_libpng():
                args += libpng_ldflags()
            return args
        else:
            # this makes the libraries suitable for linking with a path -
            # i.e. from path_of('_tl'). Without this option, the path
            # will be included in the reference and at runtime the loaded
            # will look for the path-qualified library. But that's the
            # build path and the loader will fail.
            args = []
            args += ["-Wl,-soname," + self.libname_of(mod, is_lib=True)]
            if "_dbpi" not in mod:
                loader_path = "$ORIGIN"
            else:
                loader_path = "$ORIGIN/.."
            args += ["-Wl,-rpath," + loader_path]
            # default linux shared object compilation uses the '-g' flag,
            # which generates unnecessary debug information
            # removing with strip-all during the linking stage
            args += ["-Wl,--strip-all"]
            return args

    def macros(self):
        """
        Returns the macros to use for building
        """
        macros = [
            ("HAVE_CURL", 1),
            ("HAVE_EXPAT", 1),
            ("HAVE_PNG", 1),
            ("KLAYOUT_VERSION", self.version()),
            ("KLAYOUT_MAJOR_VERSION", self.major_version()),
            ("KLAYOUT_MINOR_VERSION", self.minor_version()),
            ("GSI_ALIAS_INSPECT", 1),
        ]

        return macros

    def extra_macros(self, mod):
        """
        Returns extra module-dependent macros
        """
        if mod == "_lstream_dbpi":
            return [("CAPNP_LITE", 1)]
        else:
            return []

    def minor_version(self):
        """
        Gets the version string
        """

        # this will obtain the version string from the "version.sh" file which
        # is the central point of configuration
        version_file = os.path.join(os.path.dirname(__file__), "version.sh")
        with open(version_file, "r") as file:
            version_txt = file.read()
            rm = re.search(
                r"KLAYOUT_VERSION\s*=\s*\"(.*?)\.(.*?)(\..*)?\".*", version_txt
            )
            if rm:
                version_string = rm.group(2)
                return version_string

        raise RuntimeError("Unable to obtain version string from version.sh")

    def major_version(self):
        """
        Gets the version string
        """

        # this will obtain the version string from the "version.sh" file which
        # is the central point of configuration
        version_file = os.path.join(os.path.dirname(__file__), "version.sh")
        with open(version_file, "r") as file:
            version_txt = file.read()
            rm = re.search(
                r"KLAYOUT_VERSION\s*=\s*\"(.*?)\.(.*?)(\..*)?\".*", version_txt
            )
            if rm:
                version_string = rm.group(1)
                return version_string

        raise RuntimeError("Unable to obtain version string from version.sh")

    def version(self):
        """
        Gets the version string
        """

        # this will obtain the version string from the "version.sh" file which
        # is the central point of configuration
        version_file = os.path.join(os.path.dirname(__file__), "version.sh")
        with open(version_file, "r") as file:
            version_txt = file.read()
            rm = re.search(r"KLAYOUT_PYPI_VERSION\s*=\s*\"(.*?)\".*", version_txt)
            if rm:
                version_string = rm.group(1)
                return version_string

        raise RuntimeError("Unable to obtain version string from version.sh")


config = Config()

# ------------------------------------------------------------------
# _tl dependency library

config.module("_tl", os.path.join("src", "tl", "tl"))

_tl = Library(
    config.root + "._tl",
    define_macros=config.macros() + [("MAKE_TL_LIBRARY", 1)],
    language="c++",
    libraries=config.libraries("_tl"),
    extra_link_args=config.link_args("_tl"),
    extra_compile_args=config.compile_args("_tl"),
    sources=config.sources("_tl")
)

config.add_extension(_tl)

# ------------------------------------------------------------------
# _gsi dependency library

config.module("_gsi", os.path.join("src", "gsi", "gsi"))

_gsi = Library(
    config.root + "._gsi",
    define_macros=config.macros() + [("MAKE_GSI_LIBRARY", 1)],
    include_dirs=[config.src_path("_tl")],
    extra_objects=[config.path_of("_tl")],
    language="c++",
    libraries=config.libraries('_gsi'),
    extra_link_args=config.link_args("_gsi"),
    extra_compile_args=config.compile_args("_gsi"),
    sources=config.sources("_gsi")
)
config.add_extension(_gsi)

# ------------------------------------------------------------------
# _pya dependency library

config.module("_pya", os.path.join("src", "pya", "pya"))

_version_path = os.path.join("src", "version")

_pya = Library(
    config.root + "._pya",
    define_macros=config.macros() + [("MAKE_PYA_LIBRARY", 1)],
    include_dirs=[_version_path, config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[config.path_of("_tl"), config.path_of("_gsi")],
    language="c++",
    libraries=config.libraries("_pya"),
    extra_link_args=config.link_args("_pya"),
    extra_compile_args=config.compile_args("_pya"),
    sources=config.sources("_pya")
)
config.add_extension(_pya)

# ------------------------------------------------------------------
# _rba dependency library (dummy)

config.module("_rba", os.path.join("src", "rbastub"))

_rba = Library(
    config.root + '._rba',
    define_macros=config.macros() + [('MAKE_RBA_LIBRARY', 1)],
    include_dirs=[config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[config.path_of("_tl"), config.path_of("_gsi")],
    language="c++",
    libraries=config.libraries("_rba"),
    extra_link_args=config.link_args("_rba"),
    extra_compile_args=config.compile_args("_rba"),
    sources=config.sources("_rba")
)
config.add_extension(_rba)

# ------------------------------------------------------------------
# _db dependency library

config.module("_db", os.path.join("src", "db", "db"))

_db = Library(
    config.root + "._db",
    define_macros=config.macros() + [("MAKE_DB_LIBRARY", 1)],
    include_dirs=[config.src_path("_tl"), config.src_path("_gsi"), config.src_path("_db")],
    extra_objects=[config.path_of("_tl"), config.path_of("_gsi")],
    language="c++",
    libraries=config.libraries("_db"),
    extra_link_args=config.link_args("_db"),
    extra_compile_args=config.compile_args("_db"),
    sources=config.sources("_db")
)
config.add_extension(_db)

# ------------------------------------------------------------------
# _pex dependency library

config.module("_pex", os.path.join("src", "pex", "pex"))

_pex = Library(
    config.root + "._pex",
    define_macros=config.macros() + [("MAKE_PEX_LIBRARY", 1)],
    include_dirs=[config.src_path("_tl"), config.src_path("_gsi"), config.src_path("_db"), config.src_path("_pex")],
    extra_objects=[config.path_of("_tl"), config.path_of("_gsi"), config.path_of("_db")],
    language="c++",
    libraries=config.libraries("_pex"),
    extra_link_args=config.link_args("_pex"),
    extra_compile_args=config.compile_args("_pex"),
    sources=config.sources("_pex")
)
config.add_extension(_pex)

# ------------------------------------------------------------------
# _lib dependency library

config.module("_lib", os.path.join("src", "lib", "lib"))

_lib = Library(
    config.root + "._lib",
    define_macros=config.macros() + [("MAKE_LIB_LIBRARY", 1)],
    include_dirs=[config.src_path("_tl"), config.src_path("_gsi"), config.src_path("_db"), config.src_path("_lib")],
    extra_objects=[
        config.path_of("_tl"),
        config.path_of("_gsi"),
        config.path_of("_db"),
    ],
    language="c++",
    libraries=config.libraries("_lib"),
    extra_link_args=config.link_args("_lib"),
    extra_compile_args=config.compile_args("_lib"),
    sources=config.sources("_lib")
)
config.add_extension(_lib)

# ------------------------------------------------------------------
# _rdb dependency library

config.module("_rdb", os.path.join("src", "rdb", "rdb"))

_rdb = Library(
    config.root + "._rdb",
    define_macros=config.macros() + [("MAKE_RDB_LIBRARY", 1)],
    include_dirs=[config.src_path("_db"), config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[
        config.path_of("_tl"),
        config.path_of("_gsi"),
        config.path_of("_db"),
    ],
    language="c++",
    libraries=config.libraries('_rdb'),
    extra_link_args=config.link_args("_rdb"),
    extra_compile_args=config.compile_args("_rdb"),
    sources=config.sources("_rdb")
)
config.add_extension(_rdb)

# ------------------------------------------------------------------
# _laybasic dependency library

config.module("_laybasic", os.path.join("src", "laybasic", "laybasic"))

_laybasic = Library(
    config.root + '._laybasic',
    define_macros=config.macros() + [('MAKE_LAYBASIC_LIBRARY', 1)],
    include_dirs=[config.src_path("_rdb"), config.src_path("_db"), config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[
        config.path_of('_rdb'),
        config.path_of('_tl'),
        config.path_of('_gsi'),
        config.path_of('_db')
    ],
    language='c++',
    libraries=config.libraries('_laybasic'),
    extra_link_args=config.link_args('_laybasic'),
    extra_compile_args=config.compile_args('_laybasic'),
    sources=config.sources("_laybasic")
)
config.add_extension(_laybasic)

# ------------------------------------------------------------------
# _layview dependency library

config.module("_layview", os.path.join("src", "layview", "layview"))

_layview = Library(
    config.root + '._layview',
    define_macros=config.macros() + [('MAKE_LAYVIEW_LIBRARY', 1)],
    include_dirs=[config.src_path("_laybasic"), config.src_path("_rdb"), config.src_path("_db"), config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[
        config.path_of('_laybasic'),
        config.path_of('_rdb'),
        config.path_of('_tl'),
        config.path_of('_gsi'),
        config.path_of('_db')
    ],
    language='c++',
    libraries=config.libraries('_layview'),
    extra_link_args=config.link_args('_layview'),
    extra_compile_args=config.compile_args('_layview'),
    sources=config.sources("_layview")
)
config.add_extension(_layview)

# ------------------------------------------------------------------
# _lym dependency library

config.module("_lym", os.path.join("src", "lym", "lym"))

_lym = Library(
    config.root + '._lym',
    define_macros=config.macros() + [('MAKE_LYM_LIBRARY', 1)],
    include_dirs=[config.src_path("_pya"), config.src_path("_rba"), config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[
        config.path_of('_rba'),
        config.path_of('_pya'),
        config.path_of('_tl'),
        config.path_of('_gsi')
    ],
    language='c++',
    libraries=config.libraries('_lym'),
    extra_link_args=config.link_args('_lym'),
    extra_compile_args=config.compile_args('_lym'),
    sources=config.sources("_lym")
)
config.add_extension(_lym)

# ------------------------------------------------------------------
# _ant dependency library

config.module("_ant", os.path.join("src", "ant", "ant"))

_ant = Library(
    config.root + '._ant',
    define_macros=config.macros() + [('MAKE_ANT_LIBRARY', 1)],
    include_dirs=[config.src_path("_laybasic"), config.src_path("_layview"), config.src_path("_rdb"), config.src_path("_db"), config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[
        config.path_of('_laybasic'),
        config.path_of('_layview'),
        config.path_of('_rdb'),
        config.path_of('_tl'),
        config.path_of('_gsi'),
        config.path_of('_db')
    ],
    language='c++',
    libraries=config.libraries('_ant'),
    extra_link_args=config.link_args('_ant'),
    extra_compile_args=config.compile_args('_ant'),
    sources=config.sources("_ant")
)
config.add_extension(_ant)

# ------------------------------------------------------------------
# _img dependency library

config.module("_img", os.path.join("src", "img", "img"))

_img = Library(
    config.root + '._img',
    define_macros=config.macros() + [('MAKE_IMG_LIBRARY', 1)],
    include_dirs=[config.src_path("_laybasic"), config.src_path("_layview"), config.src_path("_rdb"), config.src_path("_db"), config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[
        config.path_of('_laybasic'),
        config.path_of('_layview'),
        config.path_of('_rdb'),
        config.path_of('_tl'),
        config.path_of('_gsi'),
        config.path_of('_db')
    ],
    language='c++',
    libraries=config.libraries('_img'),
    extra_link_args=config.link_args('_img'),
    extra_compile_args=config.compile_args('_img'),
    sources=config.sources("_img")
)
config.add_extension(_img)

# ------------------------------------------------------------------
# _edt dependency library

config.module("_edt", os.path.join("src", "edt", "edt"))

_edt = Library(
    config.root + '._edt',
    define_macros=config.macros() + [('MAKE_EDT_LIBRARY', 1)],
    include_dirs=[config.src_path("_laybasic"), config.src_path("_layview"), config.src_path("_rdb"), config.src_path("_db"), config.src_path("_tl"), config.src_path("_gsi")],
    extra_objects=[
        config.path_of('_laybasic'),
        config.path_of('_layview'),
        config.path_of('_rdb'),
        config.path_of('_tl'),
        config.path_of('_gsi'),
        config.path_of('_db')
    ],
    language='c++',
    libraries=config.libraries('_edt'),
    extra_link_args=config.link_args('_edt'),
    extra_compile_args=config.compile_args('_edt'),
    sources=config.sources("_edt")
)
config.add_extension(_edt)

# ------------------------------------------------------------------
# dependency libraries from db_plugins

db_plugins = []

dbpi_dirs = glob.glob(os.path.join("src", "plugins", "*", "db_plugin"))
dbpi_dirs += glob.glob(os.path.join("src", "plugins", "*", "*", "db_plugin"))

for pi in dbpi_dirs:

    mod_name = "_" + os.path.split(os.path.split(pi)[-2])[-1] + "_dbpi"
    config.module(mod_name, pi)

    pi_ext = Library(
        config.root + ".db_plugins." + mod_name,
        define_macros=config.macros() + [("MAKE_DB_PLUGIN_LIBRARY", 1)] + config.extra_macros(mod_name),
        include_dirs=[
            pi,
            os.path.join("src", "plugins", "common"),
            config.src_path("_db"),
            config.src_path("_tl"),
            config.src_path("_gsi")
        ] + config.extra_include_dirs(mod_name),
        extra_objects=[
            config.path_of("_tl"),
            config.path_of("_gsi"),
            config.path_of("_db"),
        ],
        language="c++",
        extra_link_args=config.link_args(mod_name),
        extra_compile_args=config.compile_args(mod_name),
        sources=config.sources(mod_name)
    )

    db_plugins.append(pi_ext)
    config.add_extension(pi_ext)

# ------------------------------------------------------------------
# tl extension library

config.module("tlcore", os.path.join("src", "pymod", "tl"))

tl = Extension(
    config.root + ".tlcore",
    define_macros=config.macros(),
    include_dirs=[config.src_path("_tl"), config.src_path("_gsi"), config.src_path("_pya")],
    extra_objects=[
        config.path_of("_tl"),
        config.path_of("_gsi"),
        config.path_of("_pya"),
    ],
    extra_link_args=config.link_args("tlcore"),
    extra_compile_args=config.compile_args("tlcore"),
    sources=config.sources("tlcore")
)

# ------------------------------------------------------------------
# db extension library

config.module("dbcore", os.path.join("src", "pymod", "db"))

db = Extension(
    config.root + ".dbcore",
    define_macros=config.macros(),
    include_dirs=[config.src_path("_db"), config.src_path("_tl"), config.src_path("_gsi"), config.src_path("_pya")],
    extra_objects=[
        config.path_of("_db"),
        config.path_of("_tl"),
        config.path_of("_gsi"),
        config.path_of("_pya"),
    ],
    extra_link_args=config.link_args("dbcore"),
    extra_compile_args=config.compile_args("dbcore"),
    sources=config.sources("dbcore")
)

# ------------------------------------------------------------------
# pex extension library

config.module("pexcore", os.path.join("src", "pymod", "pex"))

pex = Extension(
    config.root + ".pexcore",
    define_macros=config.macros(),
    include_dirs=[config.src_path("_db"), config.src_path("_tl"), config.src_path("_gsi"), config.src_path("_pya"), config.src_path("_pex")],
    extra_objects=[
        config.path_of("_db"),
        config.path_of("_pex"),
        config.path_of("_tl"),
        config.path_of("_gsi"),
        config.path_of("_pya"),
    ],
    extra_link_args=config.link_args("pexcore"),
    extra_compile_args=config.compile_args("pexcore"),
    sources=config.sources("pexcore")
)

# ------------------------------------------------------------------
# lib extension library

config.module("libcore", os.path.join("src", "pymod", "lib"))

lib = Extension(
    config.root + ".libcore",
    define_macros=config.macros(),
    include_dirs=[config.src_path("_lib"), config.src_path("_tl"), config.src_path("_gsi"), config.src_path("_pya")],
    extra_objects=[
        config.path_of("_lib"),
        config.path_of("_tl"),
        config.path_of("_gsi"),
        config.path_of("_pya"),
    ],
    extra_link_args=config.link_args("libcore"),
    extra_compile_args=config.compile_args("libcore"),
    sources=config.sources("libcore")
)

# ------------------------------------------------------------------
# rdb extension library

config.module("rdbcore", os.path.join("src", "pymod", "rdb"))

rdb = Extension(
    config.root + ".rdbcore",
    define_macros=config.macros(),
    include_dirs=[config.src_path("_rdb"), config.src_path("_tl"), config.src_path("_gsi"), config.src_path("_pya")],
    extra_objects=[
        config.path_of("_rdb"),
        config.path_of("_tl"),
        config.path_of("_gsi"),
        config.path_of("_pya"),
    ],
    extra_link_args=config.link_args("rdbcore"),
    extra_compile_args=config.compile_args("rdbcore"),
    sources=config.sources("rdbcore")
)

# ------------------------------------------------------------------
# lay extension library

config.module("laycore", os.path.join("src", "pymod", "lay"))

lay = Extension(config.root + '.laycore',
                define_macros=config.macros(),
                include_dirs=[config.src_path("_laybasic"),
                              config.src_path("_layview"),
                              config.src_path("_img"),
                              config.src_path("_ant"),
                              config.src_path("_edt"),
                              config.src_path("_lym"),
                              config.src_path("_tl"),
                              config.src_path("_gsi"),
                              config.src_path("_pya")],
                extra_objects=[config.path_of('_laybasic'),
                               config.path_of('_layview'),
                               config.path_of('_img'),
                               config.path_of('_ant'),
                               config.path_of('_edt'),
                               config.path_of('_lym'),
                               config.path_of('_tl'),
                               config.path_of('_gsi'),
                               config.path_of('_pya')],
                extra_link_args=config.link_args('laycore'),
                extra_compile_args=config.compile_args('laycore'),
                sources=config.sources("laycore"))

# ------------------------------------------------------------------
# pya extension library (all inclusive, basis of pya module)

config.module("pyacore", os.path.join("src", "pymod", "pya"))

pya = Extension(config.root + '.pyacore',
                define_macros=config.macros(),
                include_dirs=[config.src_path("_laybasic"),
                              config.src_path("_layview"),
                              config.src_path("_lib"),
                              config.src_path("_db"),
                              config.src_path("_rdb"),
                              config.src_path("_img"),
                              config.src_path("_ant"),
                              config.src_path("_edt"),
                              config.src_path("_lym"),
                              config.src_path("_tl"),
                              config.src_path("_gsi"),
                              config.src_path("_pya")],
                extra_objects=[config.path_of('_laybasic'),
                               config.path_of('_layview'),
                               config.path_of('_lib'),
                               config.path_of('_db'),
                               config.path_of('_rdb'),
                               config.path_of('_img'),
                               config.path_of('_ant'),
                               config.path_of('_edt'),
                               config.path_of('_lym'),
                               config.path_of('_tl'),
                               config.path_of('_gsi'),
                               config.path_of('_pya')],
                extra_link_args=config.link_args('pyacore'),
                extra_compile_args=config.compile_args('pyacore'),
                sources=config.sources("pyacore"))

# ------------------------------------------------------------------
# Core setup function

if __name__ == "__main__":
    typed_file = Path(__file__).parent / "src/pymod/distutils_src/klayout/py.typed"
    typed_file.parent.mkdir(parents=True, exist_ok=True)
    typed_file.touch()
    setup(
        name=config.root,
        version=config.version(),
        license="GPL-3.0-or-later",
        description="KLayout standalone Python package",
        long_description="This package is a standalone distribution of KLayout's Python API.\n\nFor more details see here: https://www.klayout.org/klayout-pypi",
        author="Matthias Koefferlein",
        author_email="matthias@klayout.org",
        classifiers=[
            # Recommended classifiers
            "Programming Language :: Python :: 2",
            "Programming Language :: Python :: 3",
            "Operating System :: MacOS :: MacOS X",
            "Operating System :: Microsoft :: Windows",
            "Operating System :: POSIX :: Linux",
            # Optional classifiers
            "Topic :: Scientific/Engineering :: Electronic Design Automation (EDA)",
        ],
        url="https://github.com/klayout/klayout",
        packages=find_packages("src/pymod/distutils_src"),
        package_dir={
            "": "src/pymod/distutils_src"
        },  # https://github.com/pypa/setuptools/issues/230
        package_data={config.root: ["src/pymod/distutils_src/klayout/*.pyi"]},
        data_files=[(config.root, ["src/pymod/distutils_src/klayout/py.typed"])],
        include_package_data=True,
        ext_modules=[_tl, _gsi, _pya, _rba, _db, _pex, _lib, _rdb, _lym, _laybasic, _layview, _ant, _edt, _img]
            + db_plugins
            + [tl, db, pex, lib, rdb, lay, pya],
        cmdclass={'build_ext': klayout_build_ext}
    )
