
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#ifndef HDR_laySaltGrainInstallationDialog
#define HDR_laySaltGrainInstallationDialog

#include "laySalt.h"

#include <QDialog>

#include "ui_SaltGrainInstallationDialog.h"

namespace lay
{

class Salt;

/**
 *  @brief The dialog for managing the Salt ("Packages")
 */
class SaltGrainInstallationDialog
  : public QDialog, private Ui::SaltGrainInstallationDialog
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  SaltGrainInstallationDialog (QWidget *parent, lay::Salt *salt);

private slots:
  /**
   *  @brief Called when the currently selected package (grain) has changed
   */
  void current_changed ();

  /**
   *  @brief Called when the Apply button is clicked
   */
  void apply ();

  /**
   *  @brief Called when the Mark button is pressed
   */
  void mark ();

private:
  lay::Salt *mp_salt;
  lay::Salt m_salt_mine;

  lay::SaltGrain *current_grain ();
};

}

#endif
