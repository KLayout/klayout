Relevant KLayout version: 0.26.9

# 1. Introduction
This directory **`macbuild`** contains different files required for building KLayout (http://www.klayout.de/) version 0.26.1 or later for different 64-bit Mac OSXs, including:
* El Capitan  (10.11)
* Sierra      (10.12)
* High Sierra (10.13)
* Mojave      (10.14)
* Catalina    (10.15) : the primary development environment
* Big Sur     (11.0)  : under development for the future support

# 2. Qt5 Frameworks
By default, the Qt framework is "Qt5" from MacPorts (https://www.macports.org/), which is usually located under:
```
/opt/local/libexec/qt5/
```

Alternatively, you can use "Qt5" from Homebrew (https://brew.sh/), which is usually located under:
```
/usr/local/opt/qt/
```

   OR

"Qt5" from Anaconda3 (https://www.anaconda.com/), which is usually located under:
```
$HOME/opt/anaconda3/pkgs/qt-{version}
```

If you have installed Anaconda3 under $HOME/opt/anaconda3/, make a symbolic link:
```
/Applications/anaconda3/ ---> $HOME/opt/anaconda3/
```

# 3. Script language support: Ruby and Python
By default, supported script languages, i.e., Ruby and Python, are those standard ones bundled with the OS.<br>
As for Catalina (10.15),
```
$ /usr/bin/ruby -v
  ruby 2.6.3p62 (2019-04-16 revision 67580) [universal.x86_64-darwin19]

$ /usr/bin/python --version
  Python 2.7.16
```
Even in the latest OS (11.0 Big Sur) as of today (November 2020), Python 3.x is not bundled with the OS, which is why users want non-OS-standard script language support.

To meet such a requirement, the build script **`build4mac.py`** provides several possible combinations of Qt5, Ruy, and Python module.<br>
Some typical use cases are described in Section 6.

# 4. Prerequisites
You need to have:
* the latest Xcode and command-line tool kit compliant with each OS
* Qt5 package from MacPorts, Homebrew, or Anaconda3
* optionally Ruby and Python packages from MacPorts, Homebrew, or Anaconda3
#### For matching versions of Ruby and Python, please also refer to `build4mac_env.py`. ####

# 5. Command-line options of **`build4mac.py`** are as shown below.
```
---------------------------------------------------------------------------------------------------------
<< Usage of **`build4mac.py`** >>
       for building KLayout 0.26.1 or later on different Apple Mac OSX platforms.

$ [python] ./build4mac.py
   option & argument    : descriptions (refer to 'macbuild/build4mac_env.py' for details)| default value
   --------------------------------------------------------------------------------------+---------------
   [-q|--qt <type>]     : case-insensitive type=['Qt5MacPorts', 'Qt5Brew', 'Qt5Ana3']    | qt5macports
                        :   Qt5MacPorts: use Qt5 from MacPorts                           |
                        :       Qt5Brew: use Qt5 from Homebrew                           |
                        :       Qt5Ana3: use Qt5 from Anaconda3                          |
   [-r|--ruby <type>]   : case-insensitive type=['nil', 'Sys', 'MP27', 'HB27', 'Ana3']   | sys
                        :    nil: don't bind Ruby                                        |
                        :    Sys: use OS-bundled Ruby [2.0 - 2.6] depending on OS        |
                        :   MP27: use Ruby 2.7 from MacPorts                             |
                        :   HB27: use Ruby 2.7 from Homebrew                             |
                        :   Ana3: use Ruby 2.5 from Anaconda3                            |
   [-p|--python <type>] : case-insensitive type=['nil', 'Sys', 'MP38', 'HB38', 'Ana3',   | sys
                        :                        'HBAuto']                               |
                        :    nil: don't bind Python                                      |
                        :    Sys: use OS-bundled Python 2.7 [ElCapitan -- BigSur]        |
                        :   MP38: use Python 3.8 from MacPorts                           |
                        :   HB38: use Python 3.8 from Homebrew                           |
                        :   Ana3: use Python 3.7 from Anaconda3                          |
                        : HBAuto: use the latest Python 3.x auto-detected from Homebrew  |
   [-n|--noqtbinding]   : don't create Qt bindings for ruby scripts                      | disabled
   [-m|--make <option>] : option passed to 'make'                                        | '-j4'
   [-d|--debug]         : enable debug mode build                                        | disabled
   [-c|--checkcom]      : check command-line and exit without building                   | disabled
   [-y|--deploy]        : deploy executables and dylibs including Qt's Frameworks        | disabled
   [-Y|--DEPLOY]        : deploy executables and dylibs for those who built KLayout      | disabled
                        : from the source code and use the tools in the same machine     |
                        : ! After confirmation of successful build of 'klayout.app',     |
                        :   rerun this script with BOTH:                                 |
                        :     1) the same options used for building AND                  |
                        :     2) <-y|--deploy> OR <-Y|--DEPLOY>                          |
                        :   optionally with [-v|--verbose <0-3>]                         |
   [-v|--verbose <0-3>] : verbose level of `macdeployqt' (effective with -y only)        | 1
                        : 0 = no output, 1 = error/warning (default),                    |
                        : 2 = normal,    3 = debug                                       |
   [-?|--?]             : print this usage and exit                                      | disabled
-----------------------------------------------------------------------------------------+---------------
```

# 6. Use-cases
In this section, the actual file names and directory names are those obtained on macOS Catalina.<br>
On different OS, those names differ accordingly.

### 6A. Standard build using the OS-bundled Ruby and Python
1. Invoke **`build4mac.py`** with the default options:
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py
```
2. Confirm successful build (it will take about one hour depending on your machine spec).
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

#### If you build KLayout from the source code AND run it on the same machine, the "-Y" option is highly recommended. ####

### 6B. Fully MacPorts-flavored build with MacPorts Ruby 2.7 and MacPorts Python 3.8
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -q qt5macports -r mp27 -p mp38
```
2. Confirm successful build (it will take about one hour depending on your machine spec).
3. Run **`build4mac.py`** again with the same options used in 1. PLUS "-Y" to deploy executables and libraries under **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed in this step.
```
$ ./build4mac.py -q qt5macports -r mp27 -p mp38 -Y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`LW-qt5MP.pkg.macos-Catalina-release-Rmp27Pmp38`** directory, where
* "LW-"        means that this is a lightweight package.
* "qt5MP"      means that Qt5 from MacPorts is used.
* "Rmp27Pmp38" means that Ruby is 2.7 from MacPorts; Python is 3.8 from MacPorts.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.

### 6C. Fully Homebrew-flavored build with Homebrew Ruby 2.7 and Homebrew Python 3.8
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -q qt5brew -r hb27 -p hb38
```
2. Confirm successful build (it will take about one hour depending on your machine spec).
3. Run **`build4mac.py`** again with the same options used in 1. PLUS "-Y" to deploy executables and libraries under **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed in this step.
```
$ ./build4mac.py -q qt5brew -r hb27 -p hb38 -Y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`LW-qt5Brew.pkg.macos-Catalina-release-Rhb27Phb38`** directory, where
* "LW-"        means that this is a lightweight package.
* "qt5Brew"    means that Qt5 from Homebrew is used.
* "Rhb27Phb38" means that Ruby is 2.7 from Homebrew; Python is 3.8 from Homebrew.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.

### 6D. Partially Homebrew-flavored build with System Ruby and Homebrew Python 3.8
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -q qt5brew -r sys -p hb38
```
2. Confirm successful build (it will take about one hour depending on your machine spec).
3. Run **`build4mac.py`** again with the same options used in 1. PLUS "-y" to deploy executables and libraries (including Qt's framework and Python framework) under **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed in this step.
```
$ ./build4mac.py -q qt5brew -r sys -p hb38 -y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`HW-qt5Brew.pkg.macos-Catalina-release-RsysPhb38`** directory, where
* "HW-"        means that this is a heavyweight package because both Qt5 and Python are deployed.
* "qt5Brew"    means that Qt5 from Homebrew is used.
* "RsysPhb38"  means that Ruby is OS-bundled; Python is 3.8 from Homebrew.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.
### Important ###
So far, the deployment of Homebrew Ruby is not supported. <br>
Therefore, if you intend to use the "-y" option for deployment, you need to use the "-r sys" option for building.

### 6E. Fully Anaconda3-flavored build with Anaconda3 Ruby 2.5 and Anaconda3 Python 3.8
```
$ cd /where/'build.sh'/exists
$ ./build4mac.py -q qt5ana3 -r ana3 -p ana3
```
2. Confirm successful build (it will take about one hour depending on your machine spec).
3. Run **`build4mac.py`** again with the same options used in 1. PLUS "-Y" to deploy executables and libraries under **`klayout.app`** bundle.<br>
   The buddy command-line tools (strm*) will also be deployed in this step.
```
$ ./build4mac.py -q qt5ana3 -r ana3 -p ana3 -Y
```
  The application bundle **`klayout.app`** is located under:<br>
  **`LW-qt5Ana3.pkg.macos-Catalina-release-Rana3Pana3`** directory, where
* "LW-"        means that this is a lightweight package.
* "qt5Ana3"    means that Qt5 from Anaconda3 is used.
* "Rana3Pana3" means that Ruby (2.5) is from Anaconda3; Python (3.8) is from Anaconda3.
4. Copy/move the generated application bundle **`klayout.app`** to your **`/Applications`** directory for installation.
5. You may have to set `PYTHONHOME` environment variable like:
```
export PYTHONHOME=$HOME/opt/anaconda3
```

### 6F. Other combinations
Logically, several different module combinations other than 6A. through 6E. are possible, including `nil` choice.<br>
If you choose such a combination, the resultant package directory name will begin with **`EX-`** (exceptional).

----

# 7. Making a DMG installer
You can make a DMG installer using another Python script **`makeDMG4mac.py`**.
This script requires a directory generated by **`build4mac.py`** with [-y|-Y] option (refer to 6A through 6E).

1. Make a symbolic link (if it does not exist) from the parent directory (where **`build.sh`** exists) to **`makeDMG4mac.py`**, that is,
```
makeDMG4mac.py -> macbuild/makeDMG4mac.py
```
2. Invoke **`makeDMG4mac.py`** with -p and -m options, for example,
```
$ cd /where/'build.sh'/exists
$ ./makeDMG4mac.py -p ST-qt5MP.pkg.macos-Catalina-release-RsysPsys -m
```
This command will generate the two files below:<br>
* **`ST-klayout-0.26.9-macOS-Catalina-1-qt5MP-RsysPsys.dmg`**      ---(1) the main DMG file
* **`ST-klayout-0.26.9-macOS-Catalina-1-qt5MP-RsysPsys.dmg.md5`**  ---(2) MD5-value text file

# Known issues
Because we assume some specific versions of non-OS-standard Ruby and Python, updating MacPorts, Homebrew, or Anaconda3 may cause build- and link errors.<br>
In such cases, you need to update the dictionary contents of **`build4mac_env.py`**.

# Final comments
No need to say, KLayout is a great tool! <br>
With the object-oriented script language (both Ruby and Python) support, our error-prone jobs can be greatly simplified and speed-up.<br>
Building KLayout from its source code is not difficult. Try it with your favorite environment!

[End of File]
