<< Draft Version 0.003>>  Relevant KLayout version: 0.25.1

# 1. Introduction:
This directory "macbuild" contains different files required for building KLayout version 0.25 or later for different Max OSX including:
* Yosemite    (10.10)
* El Capitan  (10.11)
* Sierra      (10.12)
* High Sierra (10.13)

By default, Qt framework is "Qt5" from Mac Ports (https://www.macports.org/) which is usually located under:
```
/opt/local/libexec/qt5/
```

### IMPORTANT:
```
* Please DO NOT USE "Qt 5.10.x" which is problematic in showing your design in the main canvas.
* Please USE "Qt 5.9.x" instead.
* Building with Qt4 will lead to some compile errors.
```
Also by default, supported script languages, i.e, Ruby and Python, are those standard ones bundled with the OS.

# 2. Non-OS-standard script language support
You may want to use a non-OS-standard script language such as Python 3.6 from Aanaconda2 (https://www.anaconda.com/download/#macos) in combination with KLayout.

Since Anacoda(2) is a popular Python development environment, this is worth trying.
Unfortunately, however, some dynamic linkage problems are observed as of today.
On the other hand, Python 3.6 provided by Mac Ports is usable.
Please try this (refer to 3B below) if you feel it's useful.

# 3. Use-cases:
### 3A. Debug build using the OS-standard script languages:
1. Make a symbolic link from the parent directory (where 'build.sh' exists) to
 'build4mac.py', that is,
```
  build4mac.py -> macbuild/build4mac.py
```
2. Invoke 'build4mac.py' with appropriate options ("-d" for debug build):
``` 
$ cd /where/'build.sh'/exists
$ ./build4mac.py -d
```
3. Confirm successful build.
4. Run 'build4mac.py' again with the same options used in 2. PLUS "-y" to deploy executables and libraries under "klayout.app" bundle.
```
$ ./build4mac.py -d -y
```
5. Copy/move generated bundles ("klayout.app" and "klayout.scripts/") to your "/Applications" directory for installation.

### 3B. Release build using the non-OS-standard Ruby 2.4 and Python 3.6 both from MacPorts:
1. Make a symbolic link from the parent directory (where 'build.sh' exists) to 'build4mac.py', that is,
```
build4mac.py -> macbuild/build4mac.py
```
2. Invoke 'build4mac.py' with appropriate options:
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -r mp24 -p mp36
```
3. Confirm successful build.
4. Run 'build4mac.py' again with the same options used in 2. PLUS "-Y" to deploy executables and libraries under "klayout.app" bundle.
```
$ ./build4mac.py -r mp24 -p mp36 -Y
```
* [-Y|--DEPOLY] option deploys KLayout's dylibs and executables only.
That is, paths to other modules (Ruby, Python, and Qt5 Frameworks) are remained unchanged (absolute paths in your development environment).

5. Copy/move generated bundles ("klayout.app" and "klayout.scripts/") to your "/Applications" directory.

By: Kazzz (January 08, 2018)
[End of File] 
