#! /usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function  # to use print() of Python 3 in Python >= 2.7
import sys
import os
import shutil
import glob
import platform
import optparse
import subprocess

Variation = [ 'std', 'ports', 'brew', 'ana3' ]

Usage  = "\n"
Usage += "------------------------------------------------------------------------------------------\n"
Usage += " nightyCatalina.py [EXPERIMENTAL] \n"
Usage += "   << To execute the jobs for making KLatyout's DMGs for macOS Catalina >> \n"
Usage += "\n"
Usage += "$ [python] nightyCatalina.py \n"
Usage += "   option & argument : comment on option if any                       | default value\n"
Usage += "   -------------------------------------------------------------------+--------------\n"
Usage += "     [--build]            : build and deploy                          | disabled\n"
Usage += "     [--makedmg <srlno>]  : make DMG                                  | disabled\n"
Usage += "     [--cleandmg <srlno>] : clean DMG                                 | disabled\n"
Usage += "     [--upload <dropbox>] : upload to $HOME/Dropbox/klayout/<dropbox> | disabled\n"
Usage += "     [-?|--?]             : print this usage and exit                 | disabled\n"
Usage += "----------------------------------------------------------------------+-------------------\n"

def ParseCommandLineArguments():
  global Usage
  global Build     # operation flag
  global MakeDMG   # operation flag
  global CleanDMG  # operation flag
  global Upload    # operation flag
  global SrlDMG    # DMG serial number
  global Dropbox   # Dropbox directory

  p = optparse.OptionParser( usage=Usage )
  p.add_option( '--build',
                action='store_true',
                dest='build',
                default=False,
                help='build and deploy' )

  p.add_option( '--makedmg',
                dest='makedmg',
                help='make DMG' )

  p.add_option( '--cleandmg',
                dest='cleandmg',
                help='clean DMG' )

  p.add_option( '--upload',
                dest='upload',
                help='upload to Dropbox' )

  p.add_option( '-?', '--??',
                action='store_true',
                dest='checkusage',
                default=False,
                help='check usage (false)' )

  p.set_defaults( build      = False,
                  makedmg    = "",
                  cleandmg   = "",
                  upload     = "",
                  checkusage = False )

  opt, args = p.parse_args()
  if opt.checkusage:
    print(Usage)
    quit()

  Build    = False
  MakeDMG  = False
  CleanDMG = False
  Upload   = False

  Build = opt.build

  if not opt.makedmg == "":
    MakeDMG  = True
    CleanDMG = False
    SrlDMG   = int(opt.makedmg)

  if not opt.cleandmg == "":
    MakeDMG  = False
    CleanDMG = True
    SrlDMG   = int(opt.cleandmg)

  if not opt.upload == "":
    Upload  = True
    Dropbox = opt.upload

  if not (Build or MakeDMG or CleanDMG or Upload):
    print( "! No option selected" )
    print(Usage)
    quit()


def BuildDeploy():
  PyBuild = "./build4mac.py"

  Build = dict()
  Build["std"]   = [ '-q', 'Qt5MacPorts', '-r', 'sys',  '-p', 'sys'  ]
  Build["ports"] = [ '-q', 'Qt5MacPorts', '-r', 'MP26', '-p', 'MP37' ]
  Build["brew"]  = [ '-q', 'Qt5Brew',     '-r', 'HB27', '-p', 'HB37' ]
  Build["ana3"]  = [ '-q', 'Qt5Ana3',     '-r', 'Ana3', '-p', 'Ana3' ]

  for key in Variation:
    command1 = [ PyBuild ] + Build[key]
    if key == "std":
      command2 = [ PyBuild ] + Build[key] + ['-y']
    else:
      command2 = [ PyBuild ] + Build[key] + ['-Y']
    print(command1)
    print(command2)
    #continue

    if subprocess.call( command1, shell=False ) != 0:
      print( "", file=sys.stderr )
      print( "-----------------------------------------------------------------", file=sys.stderr )
      print( "!!! <%s>: failed to build KLayout" % PyBuild, file=sys.stderr )
      print( "-----------------------------------------------------------------", file=sys.stderr )
      print( "", file=sys.stderr )
      sys.exit(1)
    else:
      print( "", file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "### <%s>: successfully built KLayout" % PyBuild, file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "", file=sys.stderr )

    if subprocess.call( command2, shell=False ) != 0:
      print( "", file=sys.stderr )
      print( "-----------------------------------------------------------------", file=sys.stderr )
      print( "!!! <%s>: failed to deploy KLayout" % PyBuild, file=sys.stderr )
      print( "-----------------------------------------------------------------", file=sys.stderr )
      print( "", file=sys.stderr )
      sys.exit(1)
    else:
      print( "", file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "### <%s>: successfully deployed KLayout" % PyBuild, file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "", file=sys.stderr )


def DMG_Make( srlDMG ):
  PyDMG = "./makeDMG4mac.py"
  Stash = "./DMGStash"

  Pack = dict()
  Pack["std"]   = [ '-p', 'ST-qt5MP.pkg.macos-Catalina-release-RsysPsys',     '-s', '%d' % srlDMG, '-m' ]
  Pack["ports"] = [ '-p', 'LW-qt5MP.pkg.macos-Catalina-release-Rmp26Pmp37',   '-s', '%d' % srlDMG, '-m' ]
  Pack["brew"]  = [ '-p', 'LW-qt5Brew.pkg.macos-Catalina-release-Rhb27Phb37', '-s', '%d' % srlDMG, '-m' ]
  Pack["ana3"]  = [ '-p', 'LW-qt5Ana3.pkg.macos-Catalina-release-Rana3Pana3', '-s', '%d' % srlDMG, '-m' ]

  if os.path.isdir( Stash ):
    shutil.rmtree( Stash )
  os.mkdir( Stash )

  for key in Variation:
    command3 = [ PyDMG ] + Pack[key]
    print(command3)
    #continue

    if subprocess.call( command3, shell=False ) != 0:
      print( "", file=sys.stderr )
      print( "-----------------------------------------------------------------", file=sys.stderr )
      print( "!!! <%s>: failed to make KLayout DMG" % PyDMG, file=sys.stderr )
      print( "-----------------------------------------------------------------", file=sys.stderr )
      print( "", file=sys.stderr )
      sys.exit(1)
    else:
      print( "", file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "### <%s>: successfully made KLayout DMG" % PyDMG, file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "", file=sys.stderr )

    dmgs = glob.glob( "*.dmg*" )
    for item in dmgs:
      shutil.move( item, Stash )


def DMG_Clean( srlDMG ):
  PyDMG = "./makeDMG4mac.py"
  Stash = "./DMGStash"

  Pack = dict()
  Pack["std"]   = [ '-p', 'ST-qt5MP.pkg.macos-Catalina-release-RsysPsys',     '-s', '%d' % srlDMG, '-c' ]
  Pack["ports"] = [ '-p', 'LW-qt5MP.pkg.macos-Catalina-release-Rmp26Pmp37',   '-s', '%d' % srlDMG, '-c' ]
  Pack["brew"]  = [ '-p', 'LW-qt5Brew.pkg.macos-Catalina-release-Rhb27Phb37', '-s', '%d' % srlDMG, '-c' ]
  Pack["ana3"]  = [ '-p', 'LW-qt5Ana3.pkg.macos-Catalina-release-Rana3Pana3', '-s', '%d' % srlDMG, '-c' ]

  if os.path.isdir( Stash ):
    shutil.rmtree( Stash )

  for key in Variation:
    command3 = [ PyDMG ] + Pack[key]
    print(command3)
    #continue

    if subprocess.call( command3, shell=False ) != 0:
      print( "", file=sys.stderr )
      print( "-----------------------------------------------------------------", file=sys.stderr )
      print( "!!! <%s>: failed to clean KLayout DMG" % PyDMG, file=sys.stderr )
      print( "-----------------------------------------------------------------", file=sys.stderr )
      print( "", file=sys.stderr )
      sys.exit(1)
    else:
      print( "", file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "### <%s>: successfully cleaned KLayout DMG" % PyDMG, file=sys.stderr )
      print( "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", file=sys.stderr )
      print( "", file=sys.stderr )


def UploadToDropbox( targetdir ):
  Stash = "./DMGStash"

  distDir = os.environ["HOME"] + "/Dropbox/klayout/" + targetdir
  if not os.path.isdir(distDir):
    os.makedirs(distDir)

  dmgs = glob.glob( "%s/*.dmg*" % Stash )
  for item in dmgs:
    shutil.copy2( item, distDir )


def Main():
  ParseCommandLineArguments()

  if Build:
    BuildDeploy()
  elif MakeDMG:
    DMG_Make( SrlDMG )
  elif CleanDMG:
    DMG_Clean( SrlDMG )
  elif Upload:
    UploadToDropbox( Dropbox )

#===================================================================================
if __name__ == "__main__":
  Main()

#---------------
# End of file
#---------------
