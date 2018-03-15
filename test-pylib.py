#!/usr/bin/env python3
import pprint
import os
from macbuild.build4mac_util import WalkFrameworkPaths, PerformChanges, DetectChanges

bundlePath = 'qt5.pkg.macos-HighSierra-release/klayout.app'
bundleExecPathAbs = os.getcwd() + '/%s/Contents/MacOS/' % bundlePath

# rsync -a --safe-links /usr/local/opt/python/Frameworks/Python.framework/ qt5.pkg.macos-HighSierra-release/klayout.app/Contents/Frameworks/Python.framework
# cp -RL /usr/local/opt/python/Frameworks/Python.framework/Versions/3.6/lib/python3.6/site-packages qt5.pkg.macos-HighSierra-release/klayout.app/Contents/Frameworks/Python.framework/Versions/3.6/lib/python3.6/
# rm -rf qt5.pkg.macos-HighSierra-release/klayout.app/Contents/Frameworks/Python.framework/Versions/3.6/lib/python3.6/site-packages/test

pythonFrameworkPath = '%s/Contents/Frameworks/Python.framework' % bundlePath
depdict = WalkFrameworkPaths(pythonFrameworkPath)
# pp = pprint.PrettyPrinter(indent=1)
# pp.pprint(depdict)

pythonOriginalFrameworkPath = '/usr/local/opt/python/Frameworks/Python.framework'
appPythonFrameworkPath = '@executable_path/../Frameworks/Python.framework/'
PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath)], bundleExecPathAbs)

klayoutPath = bundleExecPathAbs
depdict = WalkFrameworkPaths(klayoutPath, filter_regex=r'klayout$')
PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath)], bundleExecPathAbs)

klayoutPath = bundleExecPathAbs + '../Frameworks'
depdict = WalkFrameworkPaths(klayoutPath, filter_regex=r'libklayout')
PerformChanges(depdict, [(pythonOriginalFrameworkPath, appPythonFrameworkPath)], bundleExecPathAbs)


usrLocalPath = '/usr/local/opt/'
appUsrLocalPath = '@executable_path/../Frameworks/'
depdict = WalkFrameworkPaths(pythonFrameworkPath)
PerformChanges(depdict, [(usrLocalPath, appUsrLocalPath)], bundleExecPathAbs)

# usrLocalPath = '/usr/local/lib/'
# appUsrLocalPath = '@executable_path/../Frameworks/'
# depdict = WalkFrameworkPaths(pythonFrameworkPath)
# PerformChanges(depdict, [(usrLocalPath, appUsrLocalPath)], bundleExecPathAbs)


