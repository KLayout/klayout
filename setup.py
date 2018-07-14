
from distutils.core import setup, Extension, Distribution
import glob
import os
import sysconfig

class Config(object):

  def __init__(self):

    # TODO: is this really how we get the build paths?

    build_cmd = Distribution().get_command_obj('build')
    build_cmd.finalize_options()
    self.build_platlib = build_cmd.build_platlib

    install_cmd = Distribution().get_command_obj('install')
    install_cmd.finalize_options()
    self.install_platlib = install_cmd.install_platlib

    self.ext_suffix = sysconfig.get_config_var("EXT_SUFFIX")

  def libname_of(self, mod):
    return mod + self.ext_suffix

  def path_of(self, mod):
    return os.path.join(self.build_platlib, "klayout", self.libname_of(mod))

  def rpath(self):
    return [ os.path.join(self.install_platlib, "klayout") ]

  def link_args(self, mod):
    if os.name == "nt":
      return [ ]
    else:
      # this makes the libraries suitable for linking with a path -
      # i.e. from path_of('_tl')
      return ['-Wl,-soname,' + self.libname_of(mod)]

config = Config()

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
                extra_link_args = config.link_args('_tl'),
                sources = _tl_sources)

_gsi_sources = glob.glob("src/gsi/gsi/*.cc")

_gsi = Extension('klayout._gsi', 
                define_macros = macros + [ ('MAKE_GSI_LIBRARY', 1) ],
                include_dirs = [ 'src/tl/tl' ],
                extra_objects = [ config.path_of('_tl') ],
                runtime_library_dirs = config.rpath(),
                language = 'c++',
                extra_link_args = config.link_args('_gsi'),
                sources = _gsi_sources)

_pya_sources = glob.glob("src/pya/pya/*.cc")

_pya = Extension('klayout._pya', 
                define_macros = macros + [ ('MAKE_PYA_LIBRARY', 1) ],
                include_dirs = [ 'src/tl/tl', 'src/gsi/gsi' ],
                extra_objects = [ config.path_of('_tl'), config.path_of('_gsi') ],
                runtime_library_dirs = config.rpath(),
                language = 'c++',
                extra_link_args = config.link_args('_pya'),
                sources = _pya_sources)

tl_sources = glob.glob("src/pymod/tl/*.cc")

tl = Extension('klayout.tl', 
                define_macros = macros,
                include_dirs = [ 'src/tl/tl', 'src/gsi/gsi', 'src/pya/pya' ],
                extra_objects = [ config.path_of('_tl'), config.path_of('_gsi'), config.path_of('_pya') ],
                runtime_library_dirs = config.rpath(),
                sources = tl_sources)

setup (name = 'klayout',
       version = '0.26',
       description = 'KLayout standalone Python package',
       author = 'Matthias Koefferlein',
       author_email = 'matthias@klayout.de',
       ext_modules = [ _tl, _gsi, _pya, tl ])
