import site; print(site.getsitepackages())

import pip
installed_packages = pip.get_installed_distributions()
installed_packages_list = sorted(["%s==%s" % (i.key, i.version)
     for i in installed_packages])
print(installed_packages_list)

print("-------------")

if pip.main(['install', '--upgrade', 'numpy']) > 0:
    exit(1)
print("-------------")


print("Importing numpy")
import numpy
print("-------------")


import sys;
print("Executing from: ", sys.executable)
print("-------------")

import os
print("Environment variables:")
for variable, value in os.environ.items():
    print(variable, ":", value)

