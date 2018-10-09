import site; print(site.getsitepackages())

def get_pip_main():
    import pip
    # check if pip version is new:
    if int(pip.__version__.split('.')[0]) > 9:
        from pip._internal import main
    else:
        from pip import main
    return main

import sys
print("Executing from: ", sys.executable)
print("-------------")

import os
print("Environment variables:")
for variable, value in os.environ.items():
    if variable == 'DROPBOX_OAUTH_BEARER':
        continue
    print(variable, ":", value)

pipmain = get_pip_main()
print("-------------")
if pipmain(['install', '--upgrade', 'numpy']) > 0:
    exit(1)
print("-------------")


print("Importing numpy")
import numpy
print("-------------")
