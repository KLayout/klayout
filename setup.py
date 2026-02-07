"""

KLayout standalone Python module setup script


    Copyright (C) 2006-2026 Matthias Koefferlein

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
import tomli

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

        self.src_paths = { }
        self.modules = []

    def module(self, name, src_path):
        """
        Declares a module with the given name and source path

        If a file called "pysetup.toml" is found in that 
        path, it is read and a Library or Extension module 
        is created.

        Structure of TOML is:
        [library] or [extension]:
          name = "<module name>"
          depends = [ list of module names this extension depends on ]
          defines = [ list of [name, value] pairs of defines for this module ]
          submodules = [ additional parent modules: klayout.<submodules>.mod_name ]
          includes = [ additional include paths, "/a/b/..." is absolute in src, "a/b/..." is relative to module source ]
          sources = [ additional source paths, see 'includes', takes '*.cc' from there ]
          cxxflags = [ additional compiler options ]
          cxxflags-gcc = [ additional compiler options for gcc ]
          cxxflags-msvc = [ additional compiler options for msvc ]
          cxxflags-win32 = [ additional compiler options for Windows ]
          cxxflags-linux = [ additional compiler options for Linux ]
          cxxflags-darwin = [ additional compiler options for MacOS ]
            Compiler option "$libpng_cflags" is replaced by the libpng build flags
          ldflags* = [ additional linker options (*=same suffixes as cxxflags) ]
            Linker option "$libpng_ldflags" is replaced by the libpng linker flags
          libraries* = [ additional libraries (*=same suffixes as cxxflags) ]
        """

        pysetup_file = os.path.join(src_path, "pysetup.toml")
        if not os.path.isfile(pysetup_file):
            raise RuntimeError("Cannot find 'pysetup.toml' in " + src_path)
          
        with open(pysetup_file, "rb") as f:
            pysetup = tomli.load(f)

        header = {}
        is_library = None
        if "library" in pysetup: 
            is_library = True
            header = pysetup["library"]
        elif "extension" in pysetup: 
            is_library = False
            header = pysetup["extension"]
        else:
            raise RuntimeError("Invalid format - library or extension section expected in " + pysetup_file)

        header_name = header.get("name", None)
        if name is not None and header_name is not None and name != header_name:
            raise RuntimeError("Module name and specified name in setup file do not match in " + pysetup_file + ": " + header_name + " vs. " + name)
        elif name is None:
            if header_name is None:
                raise RuntimeError("No module name specified in " + pysetup_file)
            name = header_name

        depends = header.get("depends", [])
        for d in depends:
            # can't resolve now -> maybe later
            if d not in self.src_paths:
                return False

        print("Adding module " + name + " from pysetup file " + pysetup_file + " ...")

        self.src_paths[name] = src_path

        defines = header.get("defines", [])
        submodules = header.get("submodules", [])
        includes = header.get("includes", [])
        source_paths = header.get("sources", [])

        mod_path = ".".join([ self.root ] + submodules + [ name ])
        include_dirs = [ self.src_path(m) for m in depends ] + [ src_path ]
        extra_objects = [ self.path_of(m) for m in depends ]
        define_macros = self.macros() + [ tuple(d) for d in defines ]

        sources = self.sources(name)
        for i in source_paths:
            p = i.split("/")
            if len(p) > 0 and p[0] == "":
                del p[0]
            else:
                p = [ src_path ] + p
            p.append("*.cc")
            sources += glob.glob(os.path.join(*p))

        for i in includes:
            p = i.split("/")
            if len(p) > 0 and p[0] == "":
                del p[0]
            else:
                p = [ src_path ] + p
            include_dirs.append(os.path.join(*p))

        if is_library:
            ext = Library(mod_path,
                          define_macros=define_macros,
                          include_dirs=include_dirs,
                          extra_objects=extra_objects,
                          language="c++",
                          libraries=self.libraries(header),
                          extra_link_args=self.link_args(header, True),
                          extra_compile_args=self.compile_args(header),
                          sources=sources)
            self.add_extension(ext)
        else:
            ext = Extension(mod_path,
                            define_macros=define_macros,
                            include_dirs=include_dirs,
                            extra_objects=extra_objects,
                            extra_link_args=self.link_args(header, False),
                            extra_compile_args=self.compile_args(header),
                            sources=sources)

        self.modules.append(ext)

        return True

    def src_path(self, name):
        return self.src_paths[name]

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
            return os.path.join(self.build_temp, self.src_path(mod), dll_name + ".lib")
        else:
            return os.path.join(self.build_platlib, self.root, self.libname_of(mod))

    def sources(self, mod):
        """
        Gets the source files for the given module and root source path
        """
        return glob.glob(os.path.join(self.src_path(mod), "*.cc"))
          
    def compile_args(self, options):
        """
        Gets additional compiler arguments
        """
        args = []
        args += options.get("cxxflags", [])
        if platform.system() == "Windows":
            bits = os.getenv("KLAYOUT_BITS")
            if bits:
                args += [
                    quote_path("-I" + os.path.join(bits, "zlib", "include")),
                    quote_path("-I" + os.path.join(bits, "ptw", "include")),
                    quote_path("-I" + os.path.join(bits, "png", "include")),
                    quote_path("-I" + os.path.join(bits, "expat", "include")),
                    quote_path("-I" + os.path.join(bits, "curl", "include")),
                ]
            args += options.get("cxxflags-msvc", [])
            args += options.get("cxxflags-win32", [])
        else:
            args = [
                "-Wno-strict-aliasing",  # Avoids many "type-punned pointer" warnings
                "-std=c++11",  # because we use unordered_map/unordered_set
            ]
            args += options.get("cxxflags-gcc", [])
            if platform.system() == "Darwin":
                args += options.get("cxxflags-darwin", [])
            else:
                args += options.get("cxxflags-linux", [])

        result = []
        for a in args:
            if a == "$libpng_cflags":
                if check_libpng():
                    result += libpng_cflags()
            else:
                result.append(a)
        return result

    def libraries(self, options):
        """
        Gets the libraries to add
        """
        libs = []
        libs += options.get("libraries", [])
        if platform.system() == "Windows":
            libs += options.get("libraries-msvc", [])
            libs += options.get("libraries-win32", [])
        else:
            libs += options.get("libraries-gcc", [])
            if platform.system() == "Darwin":
                libs += options.get("libraries-darwin", [])
            else:
                libs += options.get("libraries-linux", [])
        return libs

    def link_args(self, options, is_library):
        """
        Gets additional linker arguments
        """
        mod = options["name"]
        args = []
        args += options.get("ldflags", [])
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
            args += options.get("ldflags-msvc", [])
            args += options.get("ldflags-win32", [])
        elif platform.system() == "Darwin":
            # For the dependency modules, make sure we produce a dylib.
            # We can only link against such, but the bundles produced otherwise.
            args = []
            if is_library:
                args += [
                    "-Wl,-dylib",
                    "-Wl,-install_name,@rpath/%s" % self.libname_of(mod, is_lib=True),
                ]
            args += ["-Wl,-rpath,@loader_path/", "-Wl,-headerpad_max_install_names"]
            args += options.get("ldflags-gcc", [])
            args += options.get("ldflags-darwin", [])
        else:
            # this makes the libraries suitable for linking with a path -
            # i.e. from path_of('_tl'). Without this option, the path
            # will be included in the reference and at runtime the loaded
            # will look for the path-qualified library. But that's the
            # build path and the loader will fail.
            args = []
            args += ["-Wl,-soname," + self.libname_of(mod, is_lib=True)]
            loader_path = ["$ORIGIN"] + [".."]*len(options.get("submodules", []))
            args += ["-Wl,-rpath," + "/".join(loader_path)]
            # default linux shared object compilation uses the '-g' flag,
            # which generates unnecessary debug information
            # removing with strip-all during the linking stage
            args += ["-Wl,--strip-all"]
            args += options.get("ldflags-gcc", [])
            args += options.get("ldflags-linux", [])

        result = []
        for a in args:
            if a == "$libpng_ldflags":
                if check_libpng():
                    result += libpng_ldflags()
            else:
                result.append(a)
        return result

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

# Collect the module spec files with dependent modules coming after
# the dependencies:

mod_specs = glob.glob(os.path.join("src", "*", "pysetup.toml"))
mod_specs += glob.glob(os.path.join("src", "*", "*", "pysetup.toml"))
mod_specs += glob.glob(os.path.join("src", "*", "*", "*", "pysetup.toml"))
mod_specs += glob.glob(os.path.join("src", "*", "*", "*", "*", "pysetup.toml"))

todo = mod_specs
while len(todo) > 0:

    not_done = []
    for ms in todo:
        p = list(os.path.split(ms))
        p.pop()
        if not config.module(None, os.path.join(*p)):
            not_done.append(ms)
      
    if len(not_done) == len(todo):
        raise RuntimeError("Dependency resolution failed - circular dependencies?")

    todo = not_done

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
        ext_modules=config.modules,
        cmdclass={'build_ext': klayout_build_ext}
    )
