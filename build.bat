
@echo off
setlocal EnableDelayedExpansion

rem ----------------------------------------------------------
rem   KLAYOUT build script
rem   See build.bat -h for details
rem

rem ----------------------------------------------------------
rem parse command lines

set CONFIG=release
set HAVE_QTBINDINGS=1
set HAVE_QT=1
set HAVE_64BIT_COORD=0
set HAVE_PYTHON=1
set HAVE_RUBY=1
set MAKE_OPT=
set HAVE_CURL=0
set HAVE_EXPAT=0
set HAVE_PTHREADS=0

set arch=x64
set compiler=msvc2017
set dry_run=0
set option-qmake=qmake
set "option-bits=c:\klayout-bits"
set "option-build=%TEMP%\klayout-build"
set "option-bin=%TEMP%\klayout-bin"

set "option="
for %%a in (%*) do (
   if not defined option (
      set arg=%%a
      if "!arg:~0,1!" equ "-" (
        if "!arg!" equ "-h" (
          set "option-h=1"
          set "option="
        ) else if "!arg!" equ "--help" (
          set "option-h=1"
          set "option="
        ) else if "!arg!" equ "-help" (
          set "option-h=1"
          set "option="
        ) else if "!arg!" equ "-bits" (
          set "option=!arg!"
        ) else if "!arg!" equ "-qmake" (
          set "option=!arg!"
        ) else if "!arg!" equ "-build" (
          set "option=!arg!"
        ) else if "!arg!" equ "-bin" (
          set "option-bin=%TEMP%\klayout-bin"
          set "option=!arg!"
        ) else if "!arg!" equ "-prefix" (
          set "option-bin=%TEMP%\klayout-bin"
          set "option=!arg!"
        ) else if "!arg!" equ "-build" (
          set "option=!arg!"
        ) else if "!arg!" equ "-debug" (
          set "CONFIG=debug"
        ) else if "!arg!" equ "-release" (
          set "CONFIG=release"
        ) else if "!arg!" equ "-with-qtbinding" (
          set "HAVE_QTBINDINGS=1"
        ) else if "!arg!" equ "-without-qtbinding" (
          set "HAVE_QTBINDINGS=0"
        ) else if "!arg!" equ "-without-qt" (
          set "HAVE_QT=0"
	  set "HAVE_CURL=1"
	  set "HAVE_EXPAT=1"
	  set "HAVE_PTHREADS=1"
          set "HAVE_QTBINDINGS=0"
        ) else if "!arg!" equ "-with-64bit-coord" (
          set "HAVE_64BIT_COORD=1"
        ) else if "!arg!" equ "-without-64bit-coord" (
          set "HAVE_64BIT_COORD=0"
        ) else if "!arg!" equ "-nopython" (
          set "HAVE_PYTHON=0"
        ) else if "!arg!" equ "-noruby" (
          set "HAVE_RUBY=0"
        ) else if "!arg!" equ "-dry-run" (
          set "dry_run=1"
        ) else if "!arg!" equ "-x64" (
          set "arch=x64"
        ) else if "!arg!" equ "-x86" (
          set "arch=x86"
        )
      ) else (
        set "MAKE_OPT=!MAKE_OPT! !arg!"
      )
   ) else (
      set "option!option!=%%a"
      set "option="
   )
)

if defined option-h (
  echo build.bat - KLayout build script
  echo.
  echo Mandatory options:
  echo    -bits [path]          Path to the 3rd party binary kit
  echo    -build [path]         Path to the build directory
  echo    -bin [path]           Path to the installation directory
  echo    -prefix [path]        Same as -bin
  echo    -x86                  32 bit build
  echo    -x64                  64 bit build [default]
  echo.
  echo Other options:
  echo    -h -help --help       Show this help
  echo    -qmake [path]         Path to the qmake binary
  echo    -debug                Perform a debug build
  echo    -release              Perform a release build [the default]
  echo    -with-qtbinding       Enable Qt support for Ruby/Python [the default]
  echo    -without-qtbinding    Disable Qt support for Ruby/Python
  echo    -without-qt           Entirely Qt-free build of a tool subset
  echo    -with-64bit-coord     Enable 64bit coordinate support [experimental]
  echo    -without-64bit-coord  Disable 64bit coordinate support [default]
  echo    -nopython             Dont include Python support
  echo    -noruby               Dont include Ruby support
  echo    -dry-run              Dont actually make
  goto :eof
)

echo Analysing installation ...
echo.

rem ----------------------------------------------------------
rem locate MSVC 2017 on the system

set MSVC2017_COMPILER_INST=notfound
rem VS 2017 sets exactly one install as the "main" install, so we may find MSBuild in there.
reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v 15.0 /reg:32 >nul 2>nul
if NOT ERRORLEVEL 1 (
  for /F "tokens=1,2*" %%i in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\VisualStudio\SxS\VS7" /v 15.0 /reg:32') DO (
    if "%%i"=="15.0" (
      if exist "%%k\VC\Auxiliary\Build" (
        set "MSVC2017_COMPILER_INST=%%k\VC\Auxiliary\Build"
        set "msg=Found MSVC installation at !MSVC2017_COMPILER_INST!"
        echo !msg!
      )
    )
  )
)

if "%MSVC2017_COMPILER_INST%" == "notfound" (
  echo ERROR: Unable to find MSVC 2017 installation
  goto :eof
)
if %arch% equ x64 (
  call "%MSVC2017_COMPILER_INST%\vcvars64"
) else (
  call "%MSVC2017_COMPILER_INST%\vcvars32"
)

rem ----------------------------------------------------------
rem check the kit

if not exist "%option-bits%"\%compiler%\%arch% (
  echo ERROR: no 3rd party binary kit found in %option-bits%\%compiler%\%arch%
  goto :eof
)
if not exist "%option-bits%\%compiler%\%arch%\ruby\bin\ruby.exe" (
  echo ERROR: %option-bits% not installed properly or
  echo path does not point to architecture folder.
  echo Use -bits to specfiy the path to the 3rd party binary kit.
  goto :eof
)
echo Using bits from %option-bits%\%compiler%\%arch%

rem ----------------------------------------------------------
rem check the qmake binary

"%option-qmake%" -v >nul 2>nul
if ERRORLEVEL 1 (
  echo ERROR: Not a valid qmake: %option-qmake%
  echo Use -qmake option to specify the path to the qmake binary.
  goto :eof
)
echo Using qmake from %option-qmake%

rem ----------------------------------------------------------
rem read klayout Version

for /F "tokens=1" %%i in ('type version.sh') do (
  set line=%%i
  if "!line:~0,16!" equ "KLAYOUT_VERSION=" (
    set "version=!line:~16,100!"
    set "KLAYOUT_VERSION=!version:"=!"
  )
)

date /t >%TEMP%\klayout-build-tmp.txt
set /P KLAYOUT_VERSION_DATE=<%TEMP%\klayout-build-tmp.txt
del %TEMP%\klayout-build-tmp.txt

rem The short SHA hash of the commit
git rev-parse --short HEAD 2>nul >%TEMP%\klayout-build-tmp.txt
set /P KLAYOUT_VERSION_REV=<%TEMP%\klayout-build-tmp.txt
if ERRORLEVEL 1 (
	set "KLAYOUT_VERSION_REV=LatestSourcePackage"
)
del %TEMP%\klayout-build-tmp.txt

rem ----------------------------------------------------------
rem dump settings

echo.
echo Architecture:             %arch%
echo Compiler:                 %compiler%
echo.
echo CONFIG:                   %CONFIG%
echo KLAYOUT_VERSION:          %KLAYOUT_VERSION%
echo KLAYOUT_VERSION_DATE:     %KLAYOUT_VERSION_DATE%
echo KLAYOUT_VERSION_REV:      %KLAYOUT_VERSION_REV%
echo HAVE_QTBINDINGS:          %HAVE_QTBINDINGS%
echo HAVE_QT:                  %HAVE_QT%
echo HAVE_64BIT_COORD:         %HAVE_64BIT_COORD%
echo HAVE_PYTHON:              %HAVE_PYTHON%
echo HAVE_RUBY:                %HAVE_RUBY%
echo HAVE_CURL:                %HAVE_CURL%
echo HAVE_PTHREADS:            %HAVE_PTHREADS%
echo HAVE_EXPAT:               %HAVE_EXPAT%
echo MAKE_OPT:                 %MAKE_OPT%
echo.
echo qmake binary:             %option-qmake%
echo Build directory:          %option-build%
echo Installation directory:   %option-bin%

if %dry_run% equ 1 (
  echo.
  echo Dry run ... stopping now.
  goto :eof
)

rem ----------------------------------------------------------
rem Run qmake

set "inst_path=%~dp0"
mkdir "%option-build%" 2>nul
cd "%option-build%"
if not exist "%option-build%" (
  echo ERROR: build directory does not exists and cannot be created
  goto :eof
)

echo on
"%option-qmake%" ^
  HAVE_QT5=1 ^
  -recursive ^
  -spec win32-msvc ^
  "CONFIG+=%CONFIG%" ^
  "KLAYOUT_VERSION=%KLAYOUT_VERSION%" ^
  "KLAYOUT_VERSION_DATE=%KLAYOUT_VERSION_DATE%" ^
  "KLAYOUT_VERSION_REV=%KLAYOUT_VERSION_REV%" ^
  "HAVE_QTBINDINGS=%HAVE_QTBINDINGS%" ^
  "HAVE_QT=%HAVE_QT%" ^
  "HAVE_EXPAT=%HAVE_EXPAT%" ^
  "HAVE_CURL=%HAVE_CURL%" ^
  "HAVE_PTHREADS=%HAVE_PTHREADS%" ^
  "HAVE_RUBY=%HAVE_RUBY%" ^
  "HAVE_PYTHON=%HAVE_PYTHON%" ^
  "HAVE_64BIT_COORD=%HAVE_64BIT_COORD%" ^
  "PREFIX=%option-bin%" ^
  "BITS_PATH=%option-bits%\%compiler%\%arch%" ^
  %inst_path%\src\klayout.pro || exit /b 1 

rem start the build
nmake %MAKE_OPT% || exit /b 2

rem install the binaries
nmake install || exit /b 3

