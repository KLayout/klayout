# klayout

This repository will hold the main sources for the KLayout project.

Plugins can be included into the "plugins" directory from external sources.

For more details see http://www.klayout.org.


## Building requirements

Building on Linux:

* Qt 4.7 or later (4.6 with some restrictions), Qt 5 or Qt 6
* gcc 4.6 or later or clang 3.8 or later

Building on Windows with MSYS2:

* MSYS2 with gcc, Qt4, 5 or 6, zlib, ruby and python packages installed

Building on Windows with MSVC 2017:

* MSVC 2017
* Build requisites from klayout-kit

For more build instructions see http://www.klayout.de/build.html.

## Build options

* <b>Ruby</b>: with this option, Ruby scripts can be executed and developped within KLayout. Ruby support is detected automatically by the build script.
* <b>Python</b>: with this option, Python scripts can be executed and developped within KLayout. Python support is detected automatically by the build script.
* <b>Qt binding</b>: with this option, Qt objects are made available to Ruby and Python scripts. Qt bindings are enabled by default. Qt binding offers an option to create custom user interfaces from scripts and to interact with KLayout's main GUI. On the other hand, they provide a considerable overhead when building and running the application.
* <b>64 bit coordinate support</b>: with this option, the coordinate type used internally is extended to 64bit as compared to 32bit in the standard version. This will duplicate memory requirements for coordinate lists, but allow a larger design space. 64bit coordinate support is experimental and disabled by default.

## Building instructions (Linux)

### Plain building for Qt4, Qt5 and Qt6 (one Qt version installed)

    ./build.sh

### Building without Qt binding

    ./build.sh -without-qtbinding

### Debug build

    ./build.sh -debug

### Building with a particular Ruby version

    ./build.sh -ruby <path-to-ruby>

(path-to-ruby is the full path of the particular ruby interpreter)

### Building with a particular Python version

    ./build.sh -python <path-to-python>

(path-to-python is the full path of the particular python interpreter)

### Building with a particular Qt version

    ./build.sh -qmake <path-to-qmake>

(path-to-qmake is the full path of the particular qmake installation)

### Building with 64bit coordinate support (experimental)

    ./build.sh -with-64bit-coord

### Pass make options

    ./build.sh -j4

(for running 4 jobs in parallel)

### More options

For more options use

    ./build.sh -h

## Running the Test Suite (Linux)

Go to the build directory (i.e. "bin-release") and enter

    export TESTTMP=testtmp    # path to a directory that will hold temporary data (will be created)
    export TESTSRC=..         # path to the source directory
    ./ut_runner

For more options use

    ./ut_runner -h

## Build instructions (Windows, MSYS2)

From the MSYS2 MinGW bash (32 bit or 64 bit) use the same commands as for Linux to build the
binaries.

## Build instructions (Windows, MSVC 2017)

The combination supported and tested was Qt 5.11/MSVC 2017 64bit.
It's sufficient to install the build tools from MSVC's community edition.

A build script similar to build.sh is provided for Windows
(build.bat).

For details about this build script use

```
build.bat -h
```

For MSVC builds a number of third party libraries are required:

 * Ruby
 * Python
 * zlib
 * expat
 * curl
 * pthread-win

The "klayout-bits4msvc2017" project (https://github.com/klayoutmatthias/klayout_bits4msvc2017) targets towards providing a binary distribution for this purpose.
See the release notes there for download links. Download the .zip archive from there and unpack it to some folder, e.g. "c:\klayout-bits".

The build script needs the path to this package. "qmake" and (for obtaining the build version) "git" should be in the path. If qmake is not in the path, you can use "build.bat -qmake ..." to specify qmake's path.

Here is an example for the build.bat call:

```
build.bat -bits c:\klayout-bits
```

The 3rd party bits kit can also be used to build the Python
standalone package on setuptools. Specify the full path to the 3rd party package up to the compiler and architecture. On 64bit with the bits package installed in "c:\klayout-bits" the build call is this:

```
set KLAYOUT_BITS=c:\klayout-bits\msvc2017\x64
python setup.py build
```
