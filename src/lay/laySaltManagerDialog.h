
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

#ifndef HDR_laySaltManagerDialog
#define HDR_laySaltManagerDialog

#include <QDialog>

#include "ui_SaltManagerDialog.h"

namespace lay
{

class Salt;
class SaltGrain;
class SaltGrainPropertiesDialog;

/**
 *  @brief The dialog for managing the Salt ("Packages")
 */
class SaltManagerDialog
  : public QDialog, private Ui::SaltManagerDialog
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  SaltManagerDialog (QWidget *parent);

private slots:
  /**
   *  @brief Called when the list of packages (grains) has changed
   */
  void salt_changed ();

  /**
   *  @brief Called when the currently selected package (grain) has changed
   */
  void current_changed ();

  /**
   *  @brief Called when the "edit" button is pressed
   */
  void edit_properties ();

private:
  lay::Salt *mp_salt;
  bool m_current_changed_enabled;
  lay::SaltGrainPropertiesDialog *mp_properties_dialog;

  lay::SaltGrain *current_grain ();
};

}

#endif
