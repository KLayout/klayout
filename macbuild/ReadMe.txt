<< Draft Version 0.002>>

1. Introduction:
    This directory "macbuild" contains different files required for building KLayout
    version 0.25 or later for different Max OSX including:
        * Yosemite    (10.10)
        * El Capitan  (10.11)
        * Sierra      (10.12)
        * High Sierra (10.13)

    By default, Qt framework is Qt5 from Mac Ports (https://www.macports.org/) which
    is usually located under:
       /opt/local/libexec/qt5/

    Also by default, supported script languages, i.e, Ruby and Python, are those
    standard ones bundled with the OS.
    However, you are able to choose other options like Python from Anaconda.
        :
        : (to be updated)
        :
        :


2. How to use:
    (1) Make a symbolic link from the parent directory (where 'build.sh' exists) to
        'build4mac.py', that is,
            build4mac.py -> macbuild/build4mac.py

    (2) Invoke 'build4mac.py' with appropriate options, for example, for debug-build:
            $ cd /where/'build.sh'/exists
            $ ./build4mac.py -d

    (3) Confirm successful build.

    (4) Run 'build4mac.py' again with the same options used in (2) PLUS "-y"
        to deploy executables and libraries under "klayout.app" bundle.
            $ ./build4mac.py -d -y

[End of File]
