Relevant KLayout version: 0.28.4<br>
Author: Kazzz-S<br>
Last modified: 2023-02-01<br>

# 1. Introduction
This directory **`macbuild`** contains various files required for building KLayout (http://www.klayout.de/) version 0.28.4 or later for different 64-bit macOS, including:
* Catalina    (10.15.7) : the primary development environment
* Big Sur     (11.x)    : experimental; Apple (M1|M2) chip is not tested since the author does not own an (M1|M2) Mac
* Monterey    (12.x)    : -- ditto --
* Ventura     (13.x)    : -- ditto --

Building KLayout for the previous operating systems listed below should still be possible. <br>
However, they are not actively supported, and DMG packages are not provided.
* Mojave      (10.14)
* High Sierra (10.13)
* Sierra      (10.12)
* El Capitan  (10.11)

Throughout this document, the primary target machine is **Intel x86_64** with **macOS Catalina**.<br>
A **((Notes))** marker indicates special notes for specific operating systems.

# 2. Qt Frameworks
**((Notes))** For **Catalina**

The default Qt framework is "Qt5" from MacPorts (https://www.macports.org/), which is usually located under:
```
/opt/local/libexec/qt5/
```

**((Notes))** For **Big Sur**, **Monterey**, and **Ventura**

The default Qt framework is "Qt5" from Homebrew (https://brew.sh/), which is usually located under:
```
/usr/local/opt/qt@5/
```

You can also choose "Qt5" from Anaconda3 (https://www.anaconda.com/), which is usually located under:
```
$HOME/opt/anaconda3/pkgs/qt-{version}
```
If you have installed Anaconda3 under $HOME/opt/anaconda3/, make a symbolic link:
```
/Applications/anaconda3/ ---> $HOME/opt/anaconda3/
```

The migration work to "Qt6" is ongoing. You can try to use it; however, you will encounter some build and runtime errors.

# 3. Script language support: Ruby and Python
Earlier, by default, supported script languages, i.e., Ruby and Python, were those standard ones bundled with the OS.<br>
```
$ /usr/bin/ruby -v
  ruby 2.6.3p62 (2019-04-16 revision 67580) [universal.x86_64-darwin19]

$ /usr/bin/python --version
  Python 2.7.16
```
Note that this configuration for backward compatibility is possible only for macOS Catalina (10.15.7).<br>
In contrast, Homebrew's Ruby 3.2 and Python 3.9 are the default environment for Big Sur, Monterey, and Ventura.<br>
Since Python 2.7 is already deprecated, using MacPorts' or Homebrew's Ruby 3.2 and Python 3.9 are also recommended for Catalina.

The build script **`build4mac.py`** provides several possible combinations of Qt5, Ruy, and Python modules to accommodate such a slightly complex environment.<br>
Some typical use cases are described in Section 6.

# 4. Prerequisites
You need to have the followings:
* The latest Xcode and command-line tool kit compliant with each OS
* Qt5 package from MacPorts, Homebrew, or Anaconda3
* Optionally, Ruby and Python packages from MacPorts, Homebrew, or Anaconda3
#### For matching versions of Ruby and Python, please also refer to `build4mac_env.py`.

# 5. Command-line options of **`build4mac.py`**
**((Notes))** For **Catalina**
```
---------------------------------------------------------------------------------------------------------
<< Usage of 'build4mac.py' >>
       for building KLayout 0.28.4 or later on different Apple macOS / Mac OSX platforms.

$ [python] ./build4mac.py
   option & argument    : descriptions (refer to 'macbuild/build4mac_env.py' for details)| default value
   --------------------------------------------------------------------------------------+---------------
   [-q|--qt <type>]     : case-insensitive type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3',    | qt5macports
                        :                        'Qt6MacPorts', 'Qt6Brew']               |
                        :   Qt5MacPorts: use Qt5 from MacPorts                           |
                        :       Qt5Brew: use Qt5 from Homebrew                           |
                        :       Qt5Ana3: use Qt5 from Anaconda3                          |
                        :   Qt6MacPorts: use Qt6 from MacPorts (*)                       |
                        :       Qt6Brew: use Qt6 from Homebrew (*)                       |
                        :                        (*) migration to Qt6 is ongoing         |
   [-r|--ruby <type>]   : case-insensitive type=['nil', 'Sys', 'MP31', 'HB31', 'Ana3',   | sys
                        :                        'MP32', HB32']                          |
                        :    nil: don't bind Ruby                                        |
                        :    Sys: use OS-bundled Ruby [2.0 - 2.6] depending on OS        |
                        :   MP31: use Ruby 3.1 from MacPorts                             |
                        :   HB31: use Ruby 3.1 from Homebrew                             |
                        :   Ana3: use Ruby 3.1 from Anaconda3                            |
                        :   MP32: use Ruby 3.2 from MacPorts                             |
                        :   HB32: use Ruby 3.2 from Homebrew                             | 
   [-p|--python <type>] : case-insensitive type=['nil',  'Sys', 'MP38', 'HB38', 'Ana3',  | sys
                        :                        'MP39', HB39', 'HBAuto']                |
                        :    nil: don't bind Python                                      |
                        :    Sys: use OS-bundled Python 2.7 up to Catalina               |
                        :   MP38: use Python 3.8 from MacPorts                           |
                        :   HB38: use Python 3.8 from Homebrew                           |
                        :   Ana3: use Python 3.9 from Anaconda3                          |
                        :   MP39: use Python 3.9 from MacPorts                           |
                        :   HB39: use Python 3.9 from Homebrew                           |
                        : HBAuto: use the latest Python 3.x auto-detected from Homebrew  |
   [-P|--buildPymod]    : build and deploy Pymod (*.whl and *.egg) for LW-*.dmg          | disabled
   [-n|--noqtbinding]   : don't create Qt bindings for ruby scripts                      | disabled
   [-u|--noqtuitools]   : don't include uitools in Qt binding                            | disabled
   [-m|--make <option>] : option passed to 'make'                                        | '--jobs=4'
   [-d|--debug]         : enable debug mode build                                        | disabled
   [-c|--checkcom]      : check command-line and exit without building                   | disabled
   [-y|--deploy]        : deploy executables and dylibs, including Qt's Frameworks       | disabled
   [-Y|--DEPLOY]        : deploy executables and dylibs for those who built KLayout      | disabled
                        : from the source code and use the tools in the same machine     |
                        : ! After confirmation of the successful build of 'klayout.app', |
                        :   rerun this script with BOTH:                                 |
                        :     1) the same options used for building AND                  |
                        :     2) <-y|--deploy> OR <-Y|--DEPLOY>                          |
                        :   optionally with [-v|--verbose <0-3>]                         |
   [-v|--verbose <0-3>] : verbose level of `macdeployqt' (effective with -y only)        | 1
                        : 0 = no output, 1 = error/warning (default),                    |
                        : 2 = normal,    3 = debug                                       |
   [-?|--?]             : print this usage and exit; in zsh, quote like '-?' or '--?'    | disabled
-----------------------------------------------------------------------------------------+---------------
```

**((Notes))** For **Big Sur**, **Monterey**, and **Ventura**
```
---------------------------------------------------------------------------------------------------------
<< Usage of 'build4mac.py' >>
       for building KLayout 0.28.4 or later on different Apple macOS / Mac OSX platforms.

$ [python] ./build4mac.py
   option & argument    : descriptions (refer to 'macbuild/build4mac_env.py' for details)| default value
   --------------------------------------------------------------------------------------+---------------
   [-q|--qt <type>]     : case-insensitive type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3',    | qt5brew
                        :                        'Qt6MacPorts', 'Qt6Brew']               |
                        :   Qt5MacPorts: use Qt5 from MacPorts                           |
                        :       Qt5Brew: use Qt5 from Homebrew                           |
                        :       Qt5Ana3: use Qt5 from Anaconda3                          |
                        :   Qt6MacPorts: use Qt6 from MacPorts (*)                       |
                        :       Qt6Brew: use Qt6 from Homebrew (*)                       |
                        :                        (*) migration to Qt6 is ongoing         |
   [-r|--ruby <type>]   : case-insensitive type=['nil', 'Sys', 'MP31', 'HB31', 'Ana3',   | hb32
                        :                        'MP32', HB32']                          |
                        :    nil: don't bind Ruby                                        |
                        :    Sys: use OS-bundled Ruby [2.0 - 2.6] depending on OS        |
                        :   MP31: use Ruby 3.1 from MacPorts                             |
                        :   HB31: use Ruby 3.1 from Homebrew                             |
                        :   Ana3: use Ruby 3.1 from Anaconda3                            |
                        :   MP32: use Ruby 3.2 from MacPorts                             |
                        :   HB32: use Ruby 3.2 from Homebrew                             | 
   [-p|--python <type>] : case-insensitive type=['nil',  'Sys', 'MP38', 'HB38', 'Ana3',  | hb39
                        :                        'MP39', HB39', 'HBAuto']                |
                        :    nil: don't bind Python                                      |
                        :    Sys: use OS-bundled Python 2.7 up to Catalina               |
                        :   MP38: use Python 3.8 from MacPorts                           |
                        :   HB38: use Python 3.8 from Homebrew                           |
                        :   Ana3: use Python 3.9 from Anaconda3                          |
                        :   MP39: use Python 3.9 from MacPorts                           |
                        :   HB39: use Python 3.9 from Homebrew                           |
                        : HBAuto: use the latest Python 3.x auto-detected from Homebrew  |
   [-P|--buildPymod]    : build and deploy Pymod (*.whl and *.egg) for LW-*.dmg          | disabled
   [-n|--noqtbinding]   : don't create Qt bindings for ruby scripts                      | disabled
   [-u|--noqtuitools]   : don't include uitools in Qt binding                            | disabled
   [-m|--make <option>] : option passed to 'make'                                        | '--jobs=4'
   [-d|--debug]         : enable debug mode build                                        | disabled
   [-c|--checkcom]      : check command-line and exit without building                   | disabled
   [-y|--deploy]        : deploy executables and dylibs, including Qt's Frameworks       | disabled
   [-Y|--DEPLOY]        : deploy executables and dylibs for those who built KLayout      | disabled
                        : from the source code and use the tools in the same machine     |
                        : ! After confirmation of the successful build of 'klayout.app', |
                        :   rerun this script with BOTH:                                 |
                        :     1) the same options used for building AND                  |
                        :     2) <-y|--deploy> OR <-Y|--DEPLOY>                          |
                        :   optionally with [-v|--verbose <0-3>]                         |
   [-v|--verbose <0-3>] : verbose level of `macdeployqt' (effective with -y only)        | 1
                        : 0 = no output, 1 = error/warning (default),                    |
                        : 2 = normal,    3 = debug                                       |
   [-?|--?]             : print this usage and exit; in zsh, quote like '-?' or '--?'    | disabled
-----------------------------------------------------------------------------------------+---------------
```

# 6. Use-cases
In this section, the actual file and directory names are those obtained on macOS Catalina.<br>
On different OS, those names differ accordingly.

### 6A. Standard build using the OS-bundled Ruby and Python **((Notes))** only for Catalina
0. Install MacPorts, then install Qt5 by
```
$ sudo port install coreutils
$ sudo port install findutils
$ sudo port install qt5
```
1. Invoke **`build4mac.py`** with the default options: **((Notes))** only for Catalina
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py
```
2. Confirm successful build (it will take about one hour, depending on your machine spec).
3. Run **`build4mac.py`** again with the same options used in 1. PLUS "-y" to deploy executables and libraries (including Qt's framework) under **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed in this step.
```
$ ./build4mac.py -y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`ST-qt5MP.pkg.macos-Catalina-release-RsysPsys`** directory, where the three name parts below are important.
* "ST-"      means that this is a standard package (LW-, HW-, and EX- are other possibilities explained below).
* "qt5MP"    means that Qt5 from MacPorts is used.
* "RsysPsys" means that Ruby is OS-bundled; Python is OS-bundled.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.

If you use the "-Y" option instead of the "-y" in Step-3, the Qt5 framework is NOT deployed in the application bundle.<br>
Then the directory name will be **`LW-qt5MP.pkg.macos-Catalina-release-RsysPsys`**, where
* "LW-" means that this is a lightweight package.

#### If you build KLayout from the source code AND run it on the same machine, the "-Y" option is highly recommended.

### 6B. Fully MacPorts-flavored build with MacPorts Ruby 3.2 and MacPorts Python 3.9
0. Install MacPorts, then install Qt5, Ruby 3.2, and Python 3.9 by
```
$ sudo port install coreutils
$ sudo port install findutils
$ sudo port install qt5
$ sudo port install ruby32
$ sudo port install python39
$ sudo port install py39-pip
```
1. Invoke **`build4mac.py`** with the following options:
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -q qt5macports -r mp32 -p mp39
```
2. Confirm successful build (it will take about one hour, depending on your machine spec).
3. Rerun **`build4mac.py`** with the same options used in 1. PLUS "-Y" to deploy executables and libraries under **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed under **klayout.app/Contents/Buddy/** in this step.<br>
   If you use `--buildPymod` option in Step-1 and Step-3, the KLayout Python Module (\*.whl, \*.egg) will be built and deployed under **klayout.app/Contents/pymod-dist/**.

```
$ ./build4mac.py -q qt5macports -r mp32 -p mp39 -Y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`LW-qt5MP.pkg.macos-Catalina-release-Rmp32Pmp39`** directory, where
* "LW-"        means this is a lightweight package.
* "qt5MP"      means that Qt5 from MacPorts is used.
* "Rmp32Pmp39" means that Ruby is 3.2 from MacPorts; Python is 3.9 from MacPorts.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.

### 6C. Fully Homebrew-flavored build with Homebrew Ruby 3.2 and Homebrew Python 3.9
0. Install Homebrew, then install Qt5, Ruby 3.2, and Python 3.9 by
```
$ brew install qt@5
$ brew install ruby@3.2
$ brew install python@3.9
```
1. Invoke **`build4mac.py`** with the following options: **((Notes))** These options are the default for Big Sur, Monterey, and Ventura.
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -q qt5brew -r hb32 -p hb39
```
2. Confirm successful build (it will take about one hour, depending on your machine spec).
3. Rerun **`build4mac.py`** with the same options used in 1. PLUS "-Y" to deploy executables and libraries under **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed under **klayout.app/Contents/Buddy/** in this step.<br>
   If you use `--buildPymod` option in Step-1 and Step-3, the KLayout Python Module (\*.whl, \*.egg) will be built and deployed under **klayout.app/Contents/pymod-dist/**.

```
$ ./build4mac.py -q qt5brew -r hb32 -p hb39 -Y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`LW-qt5Brew.pkg.macos-Catalina-release-Rhb32Phb39`** directory, where
* "LW-"        means this is a lightweight package.
* "qt5Brew"    means that Qt5 from Homebrew is used.
* "Rhb32Phb39" means that Ruby is 3.2 from Homebrew; Python is 3.9 from Homebrew.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.

### 6D. Partially Homebrew-flavored build with System Ruby and Homebrew Python 3.9
0. Install Homebrew, then install Qt5 and Python 3.9 by
```
$ brew install qt@5
$ brew install python@3.9
```
1. Invoke **`build4mac.py`** with the following options:
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -q qt5brew -r sys -p hb39
```
2. Confirm successful build (it will take about one hour, depending on your machine spec).
3. Rerun **`build4mac.py`** with the same options used in 1. PLUS "-y" to deploy executables and libraries (including Qt and Python frameworks) under the **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed under **klayout.app/Contents/Buddy/** in this step.

```
$ ./build4mac.py -q qt5brew -r sys -p hb39 -y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`HW-qt5Brew.pkg.macos-Catalina-release-RsysPhb39`** directory, where
* "HW-"        means this is a heavyweight package because both Qt5 and Python Frameworks are deployed.
* "qt5Brew"    means that Qt5 from Homebrew is used.
* "RsysPhb39"  means that Ruby is OS-bundled; Python is 3.9 from Homebrew.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.
### Important
So far, the deployment of Homebrew Ruby is not supported. <br>
Therefore, if you intend to use the "-y" option for deployment, you need to use the "-r sys" option for building.

### 6E. Fully Anaconda3-flavored build with Anaconda3 Ruby 3.1 and Anaconda3 Python 3.9
0. Install Anaconda3, then install Ruby 3.1 by
```
$ conda install ruby=3.1.2
```
1. Invoke **`build4mac.py`** with the following options:
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -q qt5ana3 -r ana3 -p ana3
```
2. Confirm successful build (it will take about one hour, depending on your machine spec).
3. Rerun **`build4mac.py`** with the same options used in 1. PLUS "-Y" to deploy executables and libraries under **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed under **klayout.app/Contents/Buddy/** in this step.<br>
   If you use `--buildPymod` option in Step-1 and Step-3, the KLayout Python Module (\*.whl, \*.egg) will be built and deployed under **klayout.app/Contents/pymod-dist/**.

```
$ ./build4mac.py -q qt5ana3 -r ana3 -p ana3 -Y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`LW-qt5Ana3.pkg.macos-Catalina-release-Rana3Pana3`** directory, where
* "LW-"        means this is a lightweight package.
* "qt5Ana3"    means that Qt5 from Anaconda3 is used.
* "Rana3Pana3" means that Ruby (3.1) is from Anaconda3; Python (3.9) is from Anaconda3.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.
5. You may have to set the `PYTHONHOME` environment variable like:
```
export PYTHONHOME=$HOME/opt/anaconda3
```

### 6F. Other combinations
Logically, several module combinations other than 6A through 6E are possible, including `nil` choice.<br>
The resultant package directory name will begin with **`EX-`** (exceptional) if you choose such a combination.

----

# 7. Making a DMG installer
You can make a DMG installer using another Python script **`makeDMG4mac.py`**.<br>
This script requires a directory generated by **`build4mac.py`** with the [-y|-Y] option (refer to 6A through 6D).

1. Make a symbolic link (if it does not exist) from the parent directory (where **`build.sh`** exists) to **`makeDMG4mac.py`**, that is,
```
makeDMG4mac.py -> macbuild/makeDMG4mac.py
```
2. Invoke **`makeDMG4mac.py`** with -p and -m options, for example,
```
$ cd /where/'build.sh'/exists
$ ./makeDMG4mac.py -p LW-qt5MP.pkg.macos-Catalina-release-Rmp32Pmp39 -m
```
This command will generate the two files below:<br>
* **`LW-klayout-0.28.4-macOS-Catalina-1-qt5MP-Rmp32Pmp39.dmg`**      ---(1) the main DMG file
* **`LW-klayout-0.28.4-macOS-Catalina-1-qt5MP-Rmp32Pmp39.dmg.md5`**  ---(2) MD5-value text file

# Known issues
Because we assume some specific versions of non-OS-standard Ruby and Python, updating MacPorts, Homebrew, or Anaconda3 may cause build- and link errors.<br>
In such cases, you need to update the dictionary contents of **`build4mac_env.py`**.

# Final comments
No need to say KLayout is a great tool! <br>
With the object-oriented script language (both Ruby and Python) support, our error-prone layout jobs can be significantly simplified and sped up.<br>
Building KLayout from its source code is not difficult. Try it with your favorite environment!

[End of File]
