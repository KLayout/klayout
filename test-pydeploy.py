import os, subprocess
import sys
from macbuild.build4mac_util import WalkFrameworkPaths, PerformChanges, DetectChanges
from pathlib import Path

# bundlePath = AbsMacPkgDir
bundlePath = os.getcwd() + '/qt5.pkg.macos-HighSierra-release/klayout.app'
bundleExecPathAbs = '%s/Contents/MacOS/' % bundlePath
pythonOriginalFrameworkPath = '/usr/local/opt/python/Frameworks/Python.framework'
pythonFrameworkPath = '%s/Contents/Frameworks/Python.framework' % bundlePath


print("[1] Copying Python Framework")
shell_commands = list()
shell_commands.append(f"rm -rf {pythonFrameworkPath}")
shell_commands.append(f"rsync -a --safe-links {pythonOriginalFrameworkPath}/ {pythonFrameworkPath}")
shell_commands.append(f"mkdir {pythonFrameworkPath}/Versions/3.6/lib/python3.6/site-packages/")
shell_commands.append(f"cp -RL {pythonOriginalFrameworkPath}/Versions/3.6/lib/python3.6/site-packages/{{pip*,pkg_resources,setuptools*,wheel*}} " +
                      f"{pythonFrameworkPath}/Versions/3.6/lib/python3.6/site-packages/")
shell_commands.append(f"rm -rf {pythonFrameworkPath}/Versions/3.6/lib/python3.6/test")
shell_commands.append(f"rm -rf {pythonFrameworkPath}/Versions/3.6/Resources")
shell_commands.append(f"rm -rf {pythonFrameworkPath}/Versions/3.6/bin")

for command in shell_commands:
  if subprocess.call( command, shell=True ) != 0:
    msg = "command failed: %s"
    print( msg % command, file=sys.stderr )
    exit(1)

print("[2] Relinking dylib dependencies inside Python.framework")
depdict = WalkFrameworkPaths(pythonFrameworkPath)
appPythonFrameworkPath = '@executable_path/../Frameworks/Python.framework/'
PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath)], bundleExecPathAbs)

usrLocalPath = '/usr/local/opt/'
appUsrLocalPath = '@executable_path/../Frameworks/'
depdict = WalkFrameworkPaths(pythonFrameworkPath)
PerformChanges(depdict, [(usrLocalPath, appUsrLocalPath)], bundleExecPathAbs, libdir=True)

print("[3] Relinking dylib dependencies for klayout")
klayoutPath = bundleExecPathAbs
depdict = WalkFrameworkPaths(klayoutPath, filter_regex=r'klayout$')
PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath)], bundleExecPathAbs)

libKlayoutPath = bundleExecPathAbs + '../Frameworks'
depdict = WalkFrameworkPaths(libKlayoutPath, filter_regex=r'libklayout')
PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath)], bundleExecPathAbs)

print("[4] Patching site.py, pip/, and distutils/")
site_module = f"{pythonFrameworkPath}/Versions/3.6/lib/python3.6/site.py"
with open(site_module, 'r') as site:
  buf = site.readlines()
with open(site_module, 'w') as site:
  import re
  for line in buf:
    # This will fool pip into thinking it's inside a virtual environment
    # and install new packates to the correct site-packages
    if re.match("^PREFIXES", line) is not None:
      line = line + "sys.real_prefix = sys.prefix\n"
    # do not allow installation in the user folder.
    if re.match("^ENABLE_USER_SITE", line) is not None:
      line = "ENABLE_USER_SITE = False\n"
    site.write(line)

pip_module = f"{pythonFrameworkPath}/Versions/3.6/lib/python3.6/site-packages/pip/__init__.py"
with open(pip_module, 'r') as pip:
  buf = pip.readlines()
with open(pip_module, 'w') as pip:
  import re
  for line in buf:
    # this will reject user's configuration of pip, forcing the isolated mode
    line = re.sub("return isolated$", "return isolated or True", line)
    pip.write(line)

distutilsconfig = f"{pythonFrameworkPath}/Versions/3.6/lib/python3.6/distutils/distutils.cfg"
with open(distutilsconfig, 'r') as file:
  buf = file.readlines()
with open(distutilsconfig, 'w') as file:
  import re
  for line in buf:
    # This will cause all packages to be installed to sys.prefix
    if re.match('prefix=', line) is not None:
      continue
    file.write(line)


# pythonPath = bundleExecPathAbs + '../Frameworks/Python.framework/Versions/3.6/bin/'
# # pythonOriginalPrefixPath = '/usr/local/opt/python/Frameworks/Python.framework/Versions/3.6'
# # appPythonBinPath = '@executable_path/../'
# depdict = WalkFrameworkPaths(pythonPath, filter_regex=r'python')
# print(depdict)
# PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath),
#   (Path(pythonOriginalFrameworkPath).resolve(), appPythonFrameworkPath)], bundleExecPathAbs)

# usrLocalPath = '/usr/local/lib/'
# appUsrLocalPath = '@executable_path/../Frameworks/'
# depdict = WalkFrameworkPaths(pythonFrameworkPath)
# PerformChanges(depdict, [(usrLocalPath, appUsrLocalPath)], bundleExecPathAbs)
