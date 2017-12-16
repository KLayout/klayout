<< Draft >>

This directory "macbuild" contains different files required to build KLayout
version 0.25 or later for different Max OSX including:
    * Yosemite    (10.10)
    * El Capitan  (10.11)
    * Sierra      (10.12)
    * High Sierra (10.13)

By default, Qt framework is the from Mac Ports (https://www.macports.org/) which
is usually located under:
   /opt/local/libexec/qt5/

Also by default, supported script languages, i.e,, Ruby and Python, are those
standard ones bundled with the OS.
However, you are able to choose other options like Python from Anaconda.

etc. etc.


Make a symbolic link from the parent directory like:
    build4mac.py -> macbuild/build4mac.py
then execute the Python script to build with appropriate options if required.

[End of File]
