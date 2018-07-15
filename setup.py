
from distutils.core import setup, Extension, Distribution
from distutils.command.build import build
import glob

# TODO: what is the portable way of finding the path of a 
def libname_of(mod):
  return mod + ".cpython-35m-x86_64-linux-gnu.so"

# TODO: what is the portable way of finding the path of a 
# library
# ....
def path_of(mod):
  return "build/lib.linux-x86_64-3.5/klayout/" + libname_of(mod)
# ....

# TODO: what is the portable way of getting the RPATH
def rpath():
  return ['/usr/local/lib/python3.5/dist-packages/klayout']

# TODO: should be platform specific
def link_args(mod):
  return ['-Wl,-soname,' + libname_of(mod)]

# ------------------------------------------------------------------

macros = [ ('HAVE_CURL', 1), ('HAVE_EXPAT', 1) ]

_tl_sources = glob.glob("src/tl/tl/*.cc")

# Exclude sources which are compatible with Qt only
_tl_sources.remove("src/tl/tl/tlHttpStreamQt.cc")
_tl_sources.remove("src/tl/tl/tlFileSystemWatcher.cc")
_tl_sources.remove("src/tl/tl/tlDeferredExecutionQt.cc")

_tl = Extension('klayout._tl', 
                define_macros = macros + [ ('MAKE_TL_LIBRARY', 1) ],
                language = 'c++',
		libraries = [ 'curl', 'expat' ],
                extra_link_args = link_args('_tl'),
                sources = _tl_sources)

_gsi_sources = glob.glob("src/gsi/gsi/*.cc")

_gsi = Extension('klayout._gsi', 
                define_macros = macros + [ ('MAKE_GSI_LIBRARY', 1) ],
                include_dirs = [ 'src/tl/tl' ],
                extra_objects = [ path_of('_tl') ],
                runtime_library_dirs = rpath(),
                language = 'c++',
                extra_link_args = link_args('_gsi'),
                sources = _gsi_sources)

_pya_sources = glob.glob("src/pya/pya/*.cc")

_pya = Extension('klayout._pya', 
                define_macros = macros + [ ('MAKE_PYA_LIBRARY', 1) ],
                include_dirs = [ 'src/tl/tl', 'src/gsi/gsi' ],
                extra_objects = [ path_of('_tl'), path_of('_gsi') ],
                runtime_library_dirs = rpath(),
                language = 'c++',
                extra_link_args = link_args('_pya'),
                sources = _pya_sources)

tl_sources = glob.glob("src/pymod/tl/*.cc")

tl = Extension('klayout.tl', 
                define_macros = macros,
                include_dirs = [ 'src/tl/tl', 'src/gsi/gsi', 'src/pya/pya' ],
                extra_objects = [ path_of('_tl'), path_of('_gsi'), path_of('_pya') ],
                runtime_library_dirs = rpath(),
                sources = tl_sources)

setup (name = 'KLayout',
       version = '0.26',
       description = 'KLayout standalone Python package',
       author = 'Matthias Koefferlein',
       author_email = 'matthias@klayout.de',
       ext_modules = [ _tl, _gsi, _pya, tl ])
