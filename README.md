# klayout

This repository will hold the main sources for the KLayout project.

Plugins can be included into the "plugins" directory from external sources.

For more details see http://www.klayout.org.


## Building requirements

* Qt 4.7 or later (4.6 with some restrictions) or Qt 5
* gcc 4.6 or later or clang 3.8 or later

For more build instructions see http://www.klayout.de/build.html.

## Build options

* <b>Ruby</b>: with this option, Ruby scripts can be executed and developped within KLayout. Ruby support is detected automatically by the build script.
* <b>Python</b>: with this option, Python scripts can be executed and developped within KLayout. Python support is detected automatically by the build script.
* <b>Qt binding</b>: with this option, Qt objects are made available to Ruby and Python scripts. Qt bindings are enabled by default. Qt binding offers an option to create custom user interfaces from scripts and to interact with KLayout's main GUI. On the other hand, they provide a considerable overhead when building and running the application.
* <b>64 bit coordinate support</b>: with this option, the coordinate type used internally is extended to 64bit as compared to 32bit in the standard version. This will duplicate memory requirements for coordinate lists, but allow a larger design space. 64bit coordinate support is experimental and disabled by default.

## Building instructions (Linux)

### Plain building for Qt4

    ./build.sh 
    
### Plain building for Qt5

    ./build.sh -qt5 
    
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


