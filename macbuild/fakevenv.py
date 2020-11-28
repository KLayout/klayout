from venv import EnvBuilder
from functools import wraps
def breakpoint_before_return(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        ret = func(*args, **kwargs)
        breakpoint()
        return ret

    return wrapper

_old_ensure_directories = EnvBuilder.ensure_directories
EnvBuilder.ensure_directories = breakpoint_before_return(_old_ensure_directories)

# Need to patch the following:
namespace(bin_name='bin',
          bin_path='/Users/tlima/02GitProjects/klayout/klayout/HW-qt5Brew.pkg.macos-Catalina-release-RsysPhbauto/klayout.app/Contents/Frameworks/Python.framework/Versions/3.8/bin',
          env_dir='/Users/tlima/02GitProjects/klayout/klayout/HW-qt5Brew.pkg.macos-Catalina-release-RsysPhbauto/klayout.app/Contents/Frameworks/Python.framework/Versions/3.8',
          env_exe='/Users/tlima/02GitProjects/klayout/klayout/HW-qt5Brew.pkg.macos-Catalina-release-RsysPhbauto/klayout.app/Contents/Frameworks/Python.framework/Versions/3.8/bin/klayout',
          env_name='3.8',
          executable='/Users/tlima/02GitProjects/klayout/klayout/HW-qt5Brew.pkg.macos-Catalina-release-RsysPhbauto/klayout.app/Contents/MacOS/klayout',
          inc_path='/Users/tlima/02GitProjects/klayout/klayout/HW-qt5Brew.pkg.macos-Catalina-release-RsysPhbauto/klayout.app/Contents/Frameworks/Python.framework/Versions/3.8/include',
          prompt='(3.8) ',
          python_dir='/Users/tlima/02GitProjects/klayout/klayout/HW-qt5Brew.pkg.macos-Catalina-release-RsysPhbauto/klayout.app/Contents/MacOS',
          python_exe='klayout')

def create(env_dir, system_site_packages=False, clear=False,
           symlinks=False, with_pip=True, prompt=None, upgrade_deps=False):
    """Create a virtual environment in a directory."""
    builder = EnvBuilder(system_site_packages=system_site_packages,
                         clear=clear, symlinks=symlinks, with_pip=with_pip,
                         prompt=prompt)

    builder.create(env_dir)

create("/Users/tlima/02GitProjects/klayout/klayout/HW-qt5Brew.pkg.macos-Catalina-release-RsysPhbauto/klayout.app/Contents/MacOS/../Frameworks/Python.framework//Versions/3.8")
